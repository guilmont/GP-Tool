#include "alignPlugin.h"

AlignPlugin::AlignPlugin(GPT::Movie *mov, GPTool *ptr) : movie(mov), tool(ptr)
{
    const GPT::Metadata &meta = movie->getMetadata();
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        data.emplace_back(meta.SizeX, meta.SizeY);

} // construct

AlignPlugin::~AlignPlugin(void) {}

void AlignPlugin::showProperties(void)
{
    ImGui::Begin("Properties");

    const uint32_t nChannels = movie->getMetadata().SizeC;

    char txt[128] = {0};
    sprintf(txt, "Channel %d", chAlign);

    if (ImGui::BeginCombo("To align", txt))
    {
        for (uint32_t ch = 1; ch < nChannels; ch++)
        {
            sprintf(txt, "Channel %d", ch);
            if (ImGui::Selectable(txt))
            {
                chAlign = ch;
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    ///////////////////////////////////////////////////////
    auto fancyDrag = [](const std::string &XYA, double &val, float reset, float step, float linewidth) -> bool
    {
        bool check = false;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        if (ImGui::Button(XYA.c_str()))
        {
            val = reset;
            check = true;
        }

        float loc = static_cast<float>(val);

        ImGui::SameLine();
        ImGui::PushItemWidth(0.4f * linewidth);
        check |= ImGui::DragFloat(("##" + XYA).c_str(), &loc, step, 0.0, 0.0, "%.4f");
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        val = static_cast<double>(loc);

        return check;
    };

    GPT::TransformData &RT = data[chAlign];

    bool check = false;
    ImGui::Columns(2);

    ImGui::Text("Translate");
    ImGui::NextColumn();
    ImGui::PushID("Translate");
    float linewidth = ImGui::GetContentRegionAvailWidth();
    check |= fancyDrag("X", RT.translate(0), 0.0f, 0.1f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", RT.translate(1), 0.0f, 0.1f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("Scale");
    ImGui::NextColumn();
    ImGui::PushID("Scale");
    check |= fancyDrag("X", RT.scale(0), 1.0f, 0.01f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", RT.scale(1), 1.0f, 0.01f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("Rotate");
    ImGui::NextColumn();
    ImGui::PushID("Rotate");
    check |= fancyDrag("X", RT.rotate(0), 0.5f * RT.size(0), 0.5f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", RT.rotate(1), 0.5f * RT.size(1), 0.5f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("Angle");
    ImGui::NextColumn();
    ImGui::PushID("Angle");
    check |= fancyDrag("A", RT.rotate(2), 0.0f, 0.005f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (check)
        RT.update();

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Save movie"))
        tool->dialog.createDialog(GDialog::SAVE, "Save TIF file...", { "tif", "ome.tif" }, this,
            [](const fs::path& path, void* ptr) -> void { std::thread(&AlignPlugin::saveTIF, reinterpret_cast<AlignPlugin*>(ptr), path).detach(); });

    ImGui::SameLine();

    if (ImGui::Button("Reset"))
    {
        const GPT::Metadata &meta = movie->getMetadata();
        data[chAlign] = GPT::TransformData(meta.SizeX, meta.SizeY);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    tool->fonts.text("Auto alignment", "bold");
    ImGui::Spacing();

    if (working)
    {
        bool v1 = camera, v2 = chromatic; // it cannot change state while working
        ImGui::Checkbox("Camera", &v1);
        ImGui::Checkbox("Chromatic aberration", &v2);
    }
    else
    {
        ImGui::Checkbox("Camera", &camera);
        ImGui::Checkbox("Chromatic aberration", &chromatic);
    }

    ImGui::Spacing();

    if (ImGui::Button("Run") && !working)
        std::thread(&AlignPlugin::runAlignment, this).detach();

    ImGui::End();

} // showProperties

void AlignPlugin::update(float deltaTime)
{
    std::array<float, 5 * 3 * 3> mat = {0};

    const uint32_t SC = movie->getMetadata().SizeC;
    uint32_t ct = 0;
    for (uint32_t ch = 0; ch < SC; ch++)
        for (uint32_t k = 0; k < 3; k++)
            for (uint32_t l = 0; l < 3; l++)
                mat[ct++] = static_cast<float>(data[ch].itrf(k, l));


    tool->shader.setMat3Array("u_align", mat.data(), 5);

} // update

///////////////////////////////////////////////////////////
// Private functions

void AlignPlugin::runAlignment(void)
{
    working = true;

    tool->mailbox.createInfo("Running alignment algorithm...");

    uint32_t nFrames = movie->getMetadata().SizeT;
    nFrames = std::min<uint32_t>(5, nFrames);

    std::vector<MatXd> vi1, vi2;
    for (uint32_t k = 0; k < nFrames; k++)
    {
        vi1.push_back(movie->getImage(0, k));
        vi2.push_back(movie->getImage(chAlign, k));
    }

    m_align = std::make_unique<GPT::Align>(nFrames, vi1.data(), vi2.data());

    if (camera)
    {
        auto clock = tool->mailbox.createTimer(
            "Correcting camera alignment...", [](void *align)
            { reinterpret_cast<GPT::Align *>(align)->stop(); },
            m_align.get());
        if (!m_align->alignCameras())
        {
            tool->mailbox.createWarn("Camera alignment didn't converge!!");
            clock->stop();
            working = false;
            return;
        }
        clock->stop();
    }

    if (chromatic)
    {
        auto clock = tool->mailbox.createTimer("Correcting chromatic aberration...", [](void *align) { reinterpret_cast<GPT::Align *>(align)->stop(); }, m_align.get());
        
        if (!m_align->correctAberrations())
        {
            tool->mailbox.createWarn("Chromatic aberration correction didn't converge!!");
            clock->stop();
            working = false;
            return;
        }
        clock->stop();
    }

    data[chAlign] = m_align->getTransformData();

    m_align.release();

    working = false;

} // runAlignement

bool AlignPlugin::saveJSON(Json::Value &json)
{

    auto jsonMat3d = [](const Mat3d &mat) -> Json::Value
    {
        Json::Value array(Json::arrayValue);
        for (uint32_t k = 0; k < 3; k++)
        {
            Json::Value row(Json::arrayValue);
            for (uint32_t l = 0; l < 3; l++)
                row.append(mat(k, l));

            array.append(std::move(row));
        }
        return array;
    };

    for (size_t ch = 1; ch < data.size(); ch++)
    {
        Json::Value align;
        const GPT::TransformData &RT = data[ch];

        align["dimensions"] = jsonArray(RT.size.data(), 2);
        align["translate"] = jsonArray(RT.translate.data(), 2);
        align["rotation"]["center"] = jsonArray(RT.rotate.data(), 2);
        align["rotation"]["angle"] = RT.rotate(2);
        align["scale"] = jsonArray(RT.scale.data(), 2);
        align["transform"] = jsonMat3d(RT.trf);

        json["channel_" + std::to_string(ch)] = std::move(align);
    }

    return true;
}

template <typename TP>
static void workOnFrame(Image<TP>* vImg, GPT::Movie* movie, const Mat3d& itrf, const uint32_t channel)
{
    const uint32_t
        nChannels = movie->getMetadata().SizeC,
        nFrames = movie->getMetadata().SizeT,
        nRows = movie->getMetadata().SizeY,
        nCols = movie->getMetadata().SizeX,
        nThreads = std::thread::hardware_concurrency();


    auto func = [&](const uint32_t tid) -> void
    {
        // Aligning second channel
        for (uint32_t frame = tid; frame < nFrames; frame += nThreads)
        {
            const uint32_t dir = frame * nChannels + channel;
            const MatXd& old = movie->getImage(channel, frame);

            Image<TP> img(nRows, nCols);
            for (uint32_t k = 0; k < nRows; k++)
                for (uint32_t l = 0; l < nCols; l++)
                {
                    int32_t x = static_cast<int32_t>(itrf(0, 0) * (l + 0.5) + itrf(0, 1) * (k + 0.5) + itrf(0, 2));
                    int32_t y = static_cast<int32_t>(itrf(1, 0) * (l + 0.5) + itrf(1, 1) * (k + 0.5) + itrf(1, 2));

                    if (x >= 0 && x < static_cast<int32_t>(nCols) && y >= 0 && y < static_cast<int32_t>(nRows))
                        img(k, l) = static_cast<TP>(old(y, x));
                    else
                        img(k, l) = 0;
                }

            vImg[dir] = img;
        }
    };


    std::vector<std::thread> vec(nThreads);
    for (uint32_t k = 0; k < nThreads; k++)
        vec[k] = std::thread(func, k);

    for (std::thread& thr : vec)
        thr.join();
    
}

void AlignPlugin::saveTIF(const fs::path& path)
{

    tool->mailbox.createInfo("Processing frames...");

    const uint32_t
        nChannels = movie->getMetadata().SizeC,
        nFrames = movie->getMetadata().SizeT,
        nDirs = nFrames * nChannels,
        nBits = movie->getMetadata().SignificantBits;
    
    if (nBits == 8)
    {
        std::vector<Image<uint8_t>> vImg(nDirs);
        for (uint32_t fr = 0; fr < nFrames; fr++)
            vImg[fr * nChannels] = movie->getImage(0, fr).cast<uint8_t>();

        for (uint32_t ch = 1; ch < nChannels; ch++)
            workOnFrame(vImg.data(), movie, data[ch].itrf, ch);


        GPT::Tiffer::Write wrt(vImg, movie->getMetadata().metaString, true);
        wrt.save(path);
    }
    
    else if (nBits == 16)
    {
        std::vector<Image<uint16_t>> vImg(nDirs);
        for (uint32_t fr = 0; fr < nFrames; fr++)
            vImg[fr * nChannels] = movie->getImage(0, fr).cast<uint16_t>();

        for (uint32_t ch = 1; ch < nChannels; ch++)
            workOnFrame(vImg.data(), movie, data[ch].itrf, ch);

        GPT::Tiffer::Write wrt(vImg, movie->getMetadata().metaString, true);
        wrt.save(path);
    }


    else if (nBits == 32)
    {
        std::vector<Image<uint32_t>> vImg(nDirs);
        for (uint32_t fr = 0; fr < nFrames; fr++)
            vImg[fr * nChannels] = movie->getImage(0, fr).cast<uint32_t>();

        for (uint32_t ch = 1; ch < nChannels; ch++)
            workOnFrame(vImg.data(), movie, data[ch].itrf, ch);

        GPT::Tiffer::Write wrt(vImg, movie->getMetadata().metaString, true);
        wrt.save(path);
    }

    else
    {
        GPT::pout("ERROR: (Alignment plugin:: save movie ==> Movie format not recognized!!");
        exit(-1);
    }

    
    tool->mailbox.createInfo("Aligned movie was saved to path: " + path.string());

}

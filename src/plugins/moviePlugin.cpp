#include "moviePlugin.h"

#include <numeric>

LUT::LUT(void)
{
    names.emplace_back("None");
    colors.emplace_back(0.0f, 0.0f, 0.0f);

    names.emplace_back("Red");
    colors.emplace_back(1.0f, 0.0f, 0.0f);

    names.emplace_back("Green");
    colors.emplace_back(0.0f, 1.0f, 0.0f);

    names.emplace_back("Blue");
    colors.emplace_back(0.0f, 0.0f, 1.0f);

    names.emplace_back("White");
    colors.emplace_back(1.0f, 1.0f, 1.0f);

    names.emplace_back("Cyan");
    colors.emplace_back(0.0f, 0.5f, 1.0f);

    names.emplace_back("Orange");
    colors.emplace_back(1.0f, 0.5f, 0.0f);
}

const glm::vec3 &LUT::getColor(const std::string &name) const
{
    uint32_t pos = 0;
    for (auto &vl : names)
    {
        if (name.compare(vl) == 0)
            return colors[pos];

        pos++;
    }

    return colors[0];
}

///////////////////////////////////////////////////////////

MoviePlugin::MoviePlugin(const std::string &movie_path, GPTool *ptr) : tool(ptr)
{
    // Importing movies
    movie = Movie(movie_path, &(tool->mbox));
    if (!movie.successful())
    {
        success = false;
        return;
    }

    uint32_t SC = movie.getMetadata().SizeC;

    // Setup info and histograms
    info.resize(SC);
    histo.resize(SC);

    for (uint32_t ch = 0; ch < SC; ch++)
    {
        info[ch].lut_name = lut.names[ch + 1];
        calcHistograms(ch);
    }
} // constructor

MoviePlugin::~MoviePlugin(void) {}

void MoviePlugin::showProperties(void)
{
    ImGui::Begin("Properties");

    const Metadata &meta = movie.getMetadata();

    tool->fonts.text("Name: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%s", meta.movie_name.c_str());

    tool->fonts.text("Acquisition date: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%s", meta.acquisitionDate.c_str());

    tool->fonts.text("Significant bits: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SignificantBits);

    tool->fonts.text("Width: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeX);

    tool->fonts.text("Height: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeY);

    tool->fonts.text("Frames: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeT);

    tool->fonts.text("Calibration: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%.3f %s", meta.PhysicalSizeXY, meta.PhysicalSizeXYUnit.c_str());

    ImGui::Spacing();
    tool->fonts.text("Channels: ", "bold");
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        ImGui::Text("   :: %s", meta.nameCH[ch].c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::SliderInt("Frame", &current_frame, 0, meta.SizeT - 1))
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
            calcHistograms(ch);

    // bool check = true;
    // check &= info.imgPos.x > 0 && info.imgPos.x < 1.0f;
    // info.imgPos.x > 0 && info.imgPos.x < 1.0f;
    // if (check && info.viewHover)
    // {
    //     float FX = meta.SizeX * info.imgPos.x,
    //           FY = meta.SizeY * info.imgPos.y;

    //     ImGui::Text("Position: %.2f x %.2f %s :: %d x %d",
    //                 meta.PhysicalSizeXY * FX, meta.PhysicalSizeXY * FY,
    //                 meta.PhysicalSizeXYUnit.c_str(),
    //                 int(FX), int(FY));
    // }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Color map:");

    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        std::string txt = "Ch " + std::to_string(ch);

        ImGui::PushID(txt.c_str());

        if (ImGui::BeginCombo(txt.c_str(), info[ch].lut_name.c_str()))
        {
            for (const std::string &name : lut.names)
            {
                bool selected = name.compare(info[ch].lut_name) == 0;
                if (ImGui::Selectable(name.c_str(), &selected))
                    info[ch].lut_name = name;

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        if (info[ch].lut_name.compare("None") != 0)
        {
            const glm::vec2 &size = histo[ch]->getSize();
            ImGui::Image((void *)(uintptr_t)histo[ch]->getID(), {size.x, size.y});

            float port = ImGui::GetContentRegionAvail().x;
            if (port != size.x)
                histo[ch] = std::make_unique<Framebuffer>(port, size.y);

            // Update contrast
            if (tool->mouse[Mouse::LEFT] == Event::PRESS && ImGui::IsItemHovered())
            {
                const ImVec2 &minPos = ImGui::GetItemRectMin(),
                             &maxPos = ImGui::GetItemRectMax(),
                             &pos = ImGui::GetMousePos();

                glm::vec2 &ct = info[ch].contrast;
                const glm::vec2 &mm = info[ch].minMaxValue;

                float dx = (pos.x - minPos.x) / (maxPos.x - minPos.x);
                dx = (mm.y - mm.x) * dx + mm.x;

                if (abs(dx - ct.x) < abs(dx - ct.y))
                    ct.x = dx;
                else
                    ct.y = dx;

            } // if-contrast Changes

        } // if-none

        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Spacing();

    } // loop-channels

    ImGui::End();
} // showProperties

void MoviePlugin::update(float deltaTime)
{
    tool->shader->useProgram("histogram");

    glm::mat4 trf = glm::ortho(-0.5f, 0.5f, 0.5f, -0.5f);
    tool->shader->setMatrix4f("u_transform", glm::value_ptr(trf));

    for (uint32_t ch = 0; ch < movie.getMetadata().SizeC; ch++)
    {

        const glm::vec3 &color = lut.getColor(info[ch].lut_name);
        tool->shader->setVec3f("color", glm::value_ptr(color));

        const glm::vec2 &ct = info[ch].contrast,
                        &mm = info[ch].minMaxValue;

        glm::vec2 contrast = {(ct.x - mm.x) / (mm.y - mm.x), (ct.y - mm.x) / (mm.y - mm.x)};
        tool->shader->setVec2f("contrast", glm::value_ptr(contrast));

        tool->shader->setFloatArray("histogram", info[ch].histogram.data(), 256);

        if (!histo[ch]) // for safety
            histo[ch] = std::make_unique<Framebuffer>(162, 100);

        histo[ch]->bind();
        tool->quad->draw();
        histo[ch]->unbind();
    }

} // update

///////////////////////////////////////////////////////////

void MoviePlugin::calcHistograms(uint32_t channel)
{

    Info *loc = &info[channel];

    const MatrixXd &mat = movie.getImage(channel, current_frame);
    float low = mat.minCoeff(), high = mat.maxCoeff(),
          minValue = 0.8f * low, maxValue = 1.2f * high;

    loc->contrast = {low, high};
    loc->minMaxValue = {minValue, maxValue};

    loc->histogram.fill(0.0f);
    for (size_t k = 0; k < mat.size(); k++)
    {
        uint32_t val = 255.0f * (mat.data()[k] - minValue) / (maxValue - minValue + 0.01f);
        loc->histogram[val]++;
    }

    float norma = std::accumulate(loc->histogram.begin(), loc->histogram.end(), 0.0f);
    for (uint32_t k = 0; k < 256; k++)
        loc->histogram[k] /= norma;

} // calcHistograms
#include "moviePlugin.h"


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
    uint64_t pos = 0;
    for (auto &vl : names)
    {
        if (name.compare(vl) == 0)
            return colors[pos];

        pos++;
    }

    return colors[0];
}

///////////////////////////////////////////////////////////

MoviePlugin::MoviePlugin(const fs::path &movie_path, GPTool *ptr) : tool(ptr)
{
    // Importing movies
    movie = std::make_unique<GPT::Movie>(movie_path);
    if (!movie->successful())
    {
        tool->mailbox.createError("Could not open movie " + movie_path.string());
        success = false;
        return;
    }

    tool->mailbox.createInfo("Movie: " + movie_path.string());

    bool running = true;
    auto prog = tool->mailbox.createProgress("Loading images...", [](void *running) { *reinterpret_cast<bool *>(running) = false; }, &running);

    // Setup info, histograms and textures
    const GPT::Metadata &meta = movie->getMetadata();

    info.resize(meta.SizeC);
    histo.resize(meta.SizeC);

    uint64_t
        counter = 0,
        nFrames = meta.SizeC * meta.SizeT;

    for (uint64_t ch = 0; ch < meta.SizeC; ch++)
    {
        float gl_low = 314159265.0f,
              gl_high = 0.0f;

        for (uint64_t fr = 0; fr < meta.SizeT; fr++)
        {
            const MatXd &mat = movie->getImage(ch, fr);
            float low = static_cast<float>(mat.minCoeff()),
                  high = static_cast<float>(mat.maxCoeff());

            gl_low = std::min(gl_low, low);
            gl_high = std::max(gl_high, high);

            prog->progress = float(++counter) / float(nFrames);

            if (!running)
            {
                success = false;
                return;
            }
        }

        float minValue = 0.8f * gl_low, maxValue = 1.2f * gl_high;

        info[ch].lut_name = lut.names[ch + uint64_t(1)];
        info[ch].contrast = {gl_low, gl_high};
        info[ch].minMaxValue = {minValue, maxValue};
    }

}


MoviePlugin::~MoviePlugin(void) {}

void MoviePlugin::updateDisplay(void)
{
    uint64_t nChannels = movie->getMetadata().SizeC;
    for (uint64_t ch = 0; ch < nChannels; ch++)
        calcHistogram(ch);
}

void MoviePlugin::showProperties(void)
{

    ImGui::Begin("Properties");

    const glm::vec2& pos = tool->viewBuf->getPosition();
    const glm::vec2& size = tool->viewBuf->getSize();
    glm::vec2 mpos = { 2.0f * (tool->mouse.position.x - pos.x) / size.x - 1.0f,
                       2.0f * (tool->mouse.position.y - pos.y) / size.y - 1.0f };

    const GPT::Metadata& meta = movie->getMetadata();
    float ratio = float(meta.SizeY) / float(meta.SizeX);
    const glm::vec3& cpos = tool->camera.position;

    mpos = { mpos.x * cpos.z + cpos.x, (mpos.y * cpos.z + cpos.y) / ratio }; // viewport reference
    mpos = { (0.5f + mpos.x) * meta.SizeX, (0.5f + mpos.y) * meta.SizeY }; // movie reference

   
    tool->fonts.text("Mouse coordinates: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%.2f x %.2f", mpos.x, mpos.y);
    ImGui::Spacing();
    ImGui::Spacing();

    //const GPT::Metadata &meta = movie->getMetadata();

    tool->fonts.text("Name: ", "bold");
    ImGui::SameLine();
    ImGui::TextUnformatted(meta.movie_name.c_str());

    tool->fonts.text("Acquisition date: ", "bold");
    ImGui::SameLine();
    ImGui::TextUnformatted(meta.acquisitionDate.c_str());

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
    for (uint64_t ch = 0; ch < meta.SizeC; ch++)
        ImGui::Text("   :: %s", meta.nameCH[ch].c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (SliderU64("Frame", &current_frame, 0, meta.SizeT - 1))
        updateDisplay();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Color map:");

    for (uint64_t ch = 0; ch < meta.SizeC; ch++)
    {
        std::string txt = "Ch " + std::to_string(ch);

        ImGui::PushID(txt.c_str());

        if (ImGui::BeginCombo(txt.c_str(), info[ch].lut_name.c_str()))
        {
            for (const std::string &name : lut.names)
            {
                bool selected = name.compare(info[ch].lut_name) == 0;
                if (ImGui::Selectable(name.c_str(), &selected))
                {
                    info[ch].lut_name = name;
                    updateTexture(ch);
                }

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
            {
                histo[ch] = std::make_unique<GRender::Framebuffer>(uint32_t(port), uint32_t(size.y));
                updateTexture(ch);
            }

            // Update contrast
            if (tool->mouse[GMouse::LEFT] == GEvent::PRESS && ImGui::IsItemHovered())
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

                updateTexture(ch);

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
    // Creating textures
    static bool firstTime = true;
    if (firstTime)
    {
        firstTime = false;
        // OpenGL on the GPU side only works after everything is setup

        // Creating quad for rendering
        quad = std::make_unique<GRender::Quad>(1);

        // To liberate already allocated textures if they exist
        tool->texture.reset();

        const GPT::Metadata &meta = movie->getMetadata();
        for (uint64_t ch = 0; ch < meta.SizeC; ch++)
        {
            uint32_t
                height = static_cast<uint32_t>(meta.SizeY),
                width = static_cast<uint32_t>(meta.SizeX);

            tool->texture.createFloat(std::to_string(ch), width, height);
            calcHistogram(ch);
        }
    }

    /////////////////////////
    // Rendring image to viewport

    const GPT::Metadata &meta = movie->getMetadata();
    glm::mat4 trf = tool->camera.getViewMatrix();
    trf = glm::scale(trf, {1.0f, float(meta.SizeY) / float(meta.SizeX), 1.0f});

    tool->viewBuf->bind();
    tool->shader.useProgram("viewport");
    tool->shader.setMatrix4f("u_transform", glm::value_ptr(trf));
    tool->shader.setInteger("u_nChannels", uint32_t(meta.SizeC));

    float size[2] = {float(meta.SizeX), float(meta.SizeY)};
    tool->shader.setVec2f("u_size", size);

    std::array<float, 15> vColor = {0.0f};
    for (uint64_t ch = 0; ch < meta.SizeC; ch++)
    {
        tool->texture.bind(std::to_string(ch), uint32_t(ch));
        const glm::vec3 &cor = lut.getColor(info[ch].lut_name);
        memcpy(vColor.data() + 3 * ch, &cor[0], 3 * sizeof(float));
    }

    tool->shader.setVec3fArray("u_color", vColor.data(), 5);

    int vTex[5] = {0, 1, 2, 3, 4};
    tool->shader.setIntArray("u_texture", vTex, 5);

    Plugin *plg = tool->getPlugin("ALIGNMENT");
    if (plg)
        plg->update(deltaTime);

    quad->draw({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f, 0.0f);
    quad->submit(); // rendering final image

    tool->viewBuf->unbind();

} // update

///////////////////////////////////////////////////////////

void MoviePlugin::calcHistogram(uint64_t channel)
{

    Info *loc = &info[channel];

    float minValue = loc->minMaxValue.x,
          maxValue = loc->minMaxValue.y;

    float low = loc->contrast.x,
          high = loc->contrast.y;

    const MatXd &mat = movie->getImage(channel, current_frame);

    loc->histogram.fill(0.0f);
    const size_t N = mat.cols() * mat.rows();
    for (size_t k = 0; k < N; k++)
    {
        float val = 255.0f * (float(mat.data()[k]) - minValue);
        val /= (maxValue - minValue + 0.01f);

        loc->histogram[uint64_t(val)]++;
    }

    float norma = std::accumulate(loc->histogram.begin(), loc->histogram.end(), 0.0f);
    for (uint64_t k = 0; k < 256; k++)
        loc->histogram[k] /= norma;

    updateTexture(channel);
}

void MoviePlugin::updateTexture(uint64_t channel)
{
    // Updating histograms
    tool->shader.useProgram("histogram");

    glm::mat4 trf = glm::ortho(-0.5f, 0.5f, 0.5f, -0.5f);
    tool->shader.setMatrix4f("u_transform", glm::value_ptr(trf));

    const glm::vec3 &color = lut.getColor(info[channel].lut_name);
    tool->shader.setVec3f("color", glm::value_ptr(color));

    const glm::vec2 &ct = info[channel].contrast,
                    &mm = info[channel].minMaxValue;

    glm::vec2 contrast = {(ct.x - mm.x) / (mm.y - mm.x), (ct.y - mm.x) / (mm.y - mm.x)};
    tool->shader.setVec2f("contrast", glm::value_ptr(contrast));

    tool->shader.setFloatArray("histogram", info[channel].histogram.data(), 256);

    if (!histo[channel]) // for safety
        histo[channel] = std::make_unique<GRender::Framebuffer>(162 * DPI_FACTOR, 100 * DPI_FACTOR);

    histo[channel]->bind();
    quad->draw({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f, 0.0f);
    quad->submit();
    histo[channel]->unbind();

    // Updating textures
    float low = info[channel].contrast.x,
          high = info[channel].contrast.y;

    // Let's use this function to update out textures for the shader
    MatXf img = movie->getImage(channel, current_frame).cast<float>();
    img = (img.array() - low) / (high - low);
    tool->texture.updateFloat(std::to_string(channel), img.data());
}

///////////////////////////////////////////////////////////////////////////////
bool MoviePlugin::saveJSON(Json::Value &json)
{
    const GPT::Metadata &meta = movie->getMetadata();
    json["movie_name"] = meta.movie_name;
    json["numChannels"] = meta.SizeC;

    for (uint64_t k = 0; k < meta.SizeC; k++)
        json["channels"].append(meta.nameCH[k]);

    return true;
}
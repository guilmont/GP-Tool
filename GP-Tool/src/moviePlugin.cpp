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

MoviePlugin::MoviePlugin(const std::string &movie_path, GPTool *ptr) : tool(ptr), firstTime(true)
{
    // Importing movies
    movie = std::make_unique<Movie>(movie_path);
    if (!movie->successful())
    {
        tool->mbox.createError("Could not open movie " + movie_path);
        success = false;
        return;
    }

    tool->mbox.createInfo("Movie: " + movie_path);

    bool running = true;
    auto prog = tool->mbox.createProgress("Loading images...", [](void *running) { *reinterpret_cast<bool *>(running) = false; }, &running);

    
    // Setup info, histograms and textures
    const Metadata &meta = movie->getMetadata();

    info.resize(meta.SizeC);
    histo.resize(meta.SizeC);

    uint32_t 
        counter = 0,
        nFrames = meta.SizeC * meta.SizeT;

    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        float gl_low = 314159265.0f,
              gl_high = 0.0f;

        for (uint32_t fr = 0; fr < meta.SizeT; fr++)
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

        info[ch].lut_name = lut.names[ch + 1];
        info[ch].contrast = {gl_low, gl_high};
        info[ch].minMaxValue = {minValue, maxValue};
    }

} // constructor

MoviePlugin::~MoviePlugin(void) {}

void MoviePlugin::showProperties(void)
{

    ImGui::Begin("Properties");

    const Metadata &meta = movie->getMetadata();

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
            calcHistogram(ch);

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
    if (firstTime)
    {
        firstTime = false;

        // To liberate already allocated textures if they exist
        tool->texture.reset();

        const Metadata &meta = movie->getMetadata();
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            tool->texture.createFloat(std::to_string(ch), meta.SizeX, meta.SizeY);
            calcHistogram(ch);
        }
    }

    const Metadata &meta = movie->getMetadata();

    glm::mat4 trf = tool->camera.getViewMatrix();
    trf = glm::scale(trf, {1.0f, float(meta.SizeY) / float(meta.SizeX), 1.0f});

    tool->shader.setMatrix4f("u_transform", glm::value_ptr(trf));
    tool->shader.setInteger("u_nChannels", meta.SizeC);

    float size[2] = {float(meta.SizeX), float(meta.SizeY)};
    tool->shader.setVec2f("u_size", size);

    std::array<float, 15> vColor = {0.0f};
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        tool->texture.bind(std::to_string(ch), ch);
        const glm::vec3 &cor = lut.getColor(info[ch].lut_name);
        memcpy(vColor.data() + 3 * ch, &cor[0], 3 * sizeof(float));
    }

    tool->shader.setVec3fArray("u_color", vColor.data(), 5);

    int vTex[5] = {0, 1, 2, 3, 4};
    tool->shader.setIntArray("u_texture", vTex, 5);

} // update

///////////////////////////////////////////////////////////

void MoviePlugin::calcHistogram(uint32_t channel)
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

        loc->histogram[uint32_t(val)]++;
    }

    float norma = std::accumulate(loc->histogram.begin(), loc->histogram.end(), 0.0f);
    for (uint32_t k = 0; k < 256; k++)
        loc->histogram[k] /= norma;

    updateTexture(channel);

}


void MoviePlugin::updateTexture(uint32_t channel)
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
    tool->quad->draw({0.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, 0.0f, 0.0f);
    tool->quad->submit();
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
    const Metadata &meta = movie->getMetadata();
    json["movie_name"] = meta.movie_name;
    json["numChannels"] = meta.SizeC;

    for (uint32_t k = 0; k < meta.SizeC; k++)
        json["channels"].append(meta.nameCH[k]);

    return true;
}
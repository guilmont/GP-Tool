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

    // Setup info, histograms and textures
    const Metadata &meta = movie.getMetadata();

    info.resize(meta.SizeC);
    histo.resize(meta.SizeC);

    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {

        const MatrixXd &mat = movie.getImage(ch, current_frame);
        float low = mat.minCoeff(), high = mat.maxCoeff(),
              minValue = 0.8f * low, maxValue = 1.2f * high;

        info[ch].lut_name = lut.names[ch + 1];
        info[ch].contrast = {low, high};
        info[ch].minMaxValue = {minValue, maxValue};
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
            calcHistogram(ch);

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
            {
                histo[ch] = std::make_unique<Framebuffer>(port, size.y);
                updateTexture(ch);
            }

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
    if (texture.empty()) // only runs the first time
    {

        const Metadata &meta = movie.getMetadata();
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            texture.emplace_back(std::make_unique<Texture>(meta.SizeX, meta.SizeY));
            calcHistogram(ch);
        }
    }

    ///////////////////////////////////////////////////////
    const Metadata &meta = movie.getMetadata();

    glm::mat4 trf = tool->camera.getViewMatrix();
    trf = glm::scale(trf, {1.0f, float(meta.SizeY) / float(meta.SizeX), 1.0f});

    tool->shader->setMatrix4f("u_transform", glm::value_ptr(trf));
    tool->shader->setInteger("u_nChannels", meta.SizeC);

    std::array<float, 15> vColor = {0.0f};
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        texture[ch]->bind(ch);
        const glm::vec3 &cor = lut.getColor(info[ch].lut_name);
        memcpy(vColor.data() + 3 * ch, &cor[0], 3 * sizeof(float));
    }

    tool->shader->setVec3fArray("u_color", vColor.data(), 5);

    int vTex[5] = {0, 1, 2, 3, 4};
    tool->shader->setIntArray("u_texture", vTex, 5);

} // update

///////////////////////////////////////////////////////////

void MoviePlugin::calcHistogram(uint32_t channel)
{

    Info *loc = &info[channel];

    float minValue = loc->minMaxValue.x,
          maxValue = loc->minMaxValue.y;

    float low = loc->contrast.x,
          high = loc->contrast.y;

    const MatrixXd &mat = movie.getImage(channel, current_frame);

    loc->histogram.fill(0.0f);
    for (size_t k = 0; k < mat.size(); k++)
    {
        uint32_t val = 255.0f * (mat.data()[k] - minValue) / (maxValue - minValue + 0.01f);
        loc->histogram[val]++;
    }

    float norma = std::accumulate(loc->histogram.begin(), loc->histogram.end(), 0.0f);
    for (uint32_t k = 0; k < 256; k++)
        loc->histogram[k] /= norma;

    updateTexture(channel);

} // calcHistograms

void MoviePlugin::updateTexture(uint32_t channel)
{

    // Updating histograms
    tool->shader->useProgram("histogram");

    glm::mat4 trf = glm::ortho(-0.5f, 0.5f, 0.5f, -0.5f);
    tool->shader->setMatrix4f("u_transform", glm::value_ptr(trf));

    const glm::vec3 &color = lut.getColor(info[channel].lut_name);
    tool->shader->setVec3f("color", glm::value_ptr(color));

    const glm::vec2 &ct = info[channel].contrast,
                    &mm = info[channel].minMaxValue;

    glm::vec2 contrast = {(ct.x - mm.x) / (mm.y - mm.x), (ct.y - mm.x) / (mm.y - mm.x)};
    tool->shader->setVec2f("contrast", glm::value_ptr(contrast));

    tool->shader->setFloatArray("histogram", info[channel].histogram.data(), 256);

    if (!histo[channel]) // for safety
        histo[channel] = std::make_unique<Framebuffer>(162, 100);

    histo[channel]->bind();
    tool->quad->draw();
    histo[channel]->unbind();

    // Updating textures
    float low = info[channel].contrast.x,
          high = info[channel].contrast.y;

    // Let's use this function to update out textures for the shader
    MatrixXf img = movie.getImage(channel, current_frame).cast<float>();
    img = (img.array() - low) / (high - low);
    texture[channel]->update(img.data());

} // calcHistograms
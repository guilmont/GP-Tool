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

MoviePlugin::MoviePlugin(const std::string &movie_path, Mailbox *mail) : mbox(mail)
{
    // Importing movies
    movie = Movie(movie_path, mail);
    if (!movie.successful())
    {
        success = false;
        return;
    }

    const Metadata &meta = movie.getMetadata();

    data.resize(meta.SizeC);
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        data[ch].lut_name = lut.names[ch + 1];

} // constructor

MoviePlugin::~MoviePlugin(void) {}

void MoviePlugin::showProperties(void)
{
    ImGui::Begin("Properties");

    const Metadata &meta = movie.getMetadata();

    ImGui::Text("Name: ");
    ImGui::SameLine();
    ImGui::Text("%s", meta.movie_name.c_str());

    ImGui::Text("Acquisition date: ");
    ImGui::SameLine();
    ImGui::Text("%s", meta.acquisitionDate.c_str());

    ImGui::Text("Significant bits: ");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SignificantBits);

    ImGui::Text("Width: ");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeX);

    ImGui::Text("Height: ");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeY);

    ImGui::Text("Frames: ");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeT);

    ImGui::Text("Calibration: ");
    ImGui::SameLine();
    ImGui::Text("%.3f %s", meta.PhysicalSizeXY, meta.PhysicalSizeXYUnit.c_str());

    ImGui::Spacing();
    ImGui::Text("Channels: ");
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        ImGui::Text("   :: %s", meta.nameCH[ch].c_str());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SliderInt("Frame", &current_frame, 0, meta.SizeT - 1);

    // bool check = true;
    // check &= data.imgPos.x > 0 && data.imgPos.x < 1.0f;
    // check &= data.imgPos.y > 0 && data.imgPos.y < 1.0f;
    // if (check && data.viewHover)
    // {
    //     float FX = meta.SizeX * data.imgPos.x,
    //           FY = meta.SizeY * data.imgPos.y;

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

        if (ImGui::BeginCombo(txt.c_str(), data[ch].lut_name.c_str()))
        {
            for (const std::string &name : lut.names)
            {
                bool selected = name.compare(data[ch].lut_name) == 0;
                if (ImGui::Selectable(name.c_str(), &selected))
                    data[ch].lut_name = name;

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        //     if (current.compare("None") != 0)
        //     {
        //         String name = "contrast_" + std::to_string(ch);

        //         float port = ImGui::GetContentRegionAvail().x,
        //               wid = ui.FBuffer[name]->getWidth(),
        //               hei = ui.FBuffer[name]->getHeight();

        //         ImGui::Image((void *)(uintptr_t)ui.FBuffer[name]->getID(), {wid, hei});

        //         if (port != wid)
        //         {
        //             wid = port;
        //             ui.FBuffer[name] = std::make_unique<Framebuffer>(wid, hei);
        //             data.updateContrast_flag = true;
        //         }

        //         // Update contrast
        //         if (ui.mouse.leftPressed && ImGui::IsItemHovered())
        //         {
        //             const ImVec2 &minPos = ImGui::GetItemRectMin(),
        //                          &maxPos = ImGui::GetItemRectMax(),
        //                          &pos = ImGui::GetMousePos();

        //             auto [low, high] = data.iprops.getContrast(ch);
        //             auto &[minValue, maxValue] = data.iprops.getMinMaxValues(ch);

        //             float dx = (pos.x - minPos.x) / (maxPos.x - minPos.x);
        //             dx = (maxValue - minValue) * dx + minValue;

        //             if (abs(dx - low) < abs(dx - high))
        //                 low = dx;
        //             else
        //                 high = dx;

        //             data.iprops.setConstrast(ch, low, high);

        //         } // if-contrastWidget Changes
        //     }

        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Spacing();

    } // loop-channels

    ImGui::End();
} // showProperties

void MoviePlugin::update(float deltaTime)
{

} // update

void MoviePlugin::setLUT(uint32_t ch, const std::string &lut_name)
{
    data[ch].lut_name = std::move(lut_name);
}

void MoviePlugin::setConstrast(uint32_t ch, const glm::vec2 &contrast)
{
    data[ch].contrast = std::move(contrast);
}

void MoviePlugin::setMinMaxValues(uint32_t ch, const glm::vec2 &values)
{
    data[ch].minMaxValue = std::move(values);
}

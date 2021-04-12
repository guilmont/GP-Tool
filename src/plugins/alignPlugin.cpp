#include "alignPlugin.h"

AlignPlugin::AlignPlugin(const Movie *mov, GPTool *ptr) : movie(mov), tool(ptr)
{
    const Metadata &meta = movie->getMetadata();
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
    auto fancyDrag = [](const String &XYA, float &val, float reset, float step,
                        float linewidth) -> bool {
        bool check = false;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
        if (ImGui::Button(XYA.c_str()))
        {
            val = reset;
            check = true;
        }

        ImGui::SameLine();
        ImGui::PushItemWidth(0.4f * linewidth);
        check |= ImGui::DragFloat(("##" + XYA).c_str(), &val, step, 0.0, 0.0, "%.3f");
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        return check;
    };

    TransformData &RT = data[chAlign];

    bool check = false;
    ImGui::Columns(2);

    ImGui::Text("Translate");
    ImGui::NextColumn();
    ImGui::PushID("Translate");
    float linewidth = ImGui::GetContentRegionAvailWidth();
    check |= fancyDrag("X", RT.translate.x, 0.0f, 0.01f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", RT.translate.y, 0.0f, 0.01f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("Scale");
    ImGui::NextColumn();
    ImGui::PushID("Scale");
    check |= fancyDrag("X", RT.scale.x, 1.0f, 0.001f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", RT.scale.y, 1.0f, 0.001f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("Rotate");
    ImGui::NextColumn();
    ImGui::PushID("Rotate");
    check |= fancyDrag("X", RT.rotate.x, 0.5f, 0.05f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", RT.rotate.y, 0.5f, 0.05f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Text("Angle");
    ImGui::NextColumn();
    ImGui::PushID("Angle");
    check |= fancyDrag("A", RT.rotate.z, 0.0f, 0.001f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (check)
        RT.update();

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Reset"))
    {
        const Metadata &meta = movie->getMetadata();
        data[chAlign] = TransformData(meta.SizeX, meta.SizeY);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    tool->fonts.text("Auto alignment", "bold");
    ImGui::Spacing();

    ImGui::Checkbox("Camera", &camera);
    ImGui::Checkbox("Chromatic aberration", &chromatic);

    ImGui::Spacing();

    if (ImGui::Button("Run"))
    {
    }

    ImGui::End();

} // showProperties

void AlignPlugin::update(float deltaTime)
{
    std::array<float, 5 * 3 * 3> mat = {0};

    const uint32_t SC = movie->getMetadata().SizeC;
    for (uint32_t ch = 0; ch < SC; ch++)
        memcpy(mat.data() + 9 * ch, glm::value_ptr(data[ch].itrf), 9 * sizeof(float));

    tool->shader->setMat3Array("u_align", mat.data(), 5);

} // update

///////////////////////////////////////////////////////////
// Private functions

void AlignPlugin::runAlignment(void)
{

    // uint32_t chAlign = data->align->getChannelIndex();

    // data->align->setImageData(data->movie->getMetadata().SizeT,
    //                           data->movie->getChannel(0),
    //                           data->movie->getChannel(chAlign));

    // MSG_Timer *msg2 = box->createMessage<MSG_Timer>("Aligning cameras");
    // bool check = data->align->alignCameras();
    // msg2->markAsRead();

    // if (check)
    // {
    //     Message *msg3 = box->createMessage<MSG_Timer>("Correcting chromatic aberrations");
    //     data->align->correctAberrations();
    //     msg3->markAsRead();
    // }

} // runAlignement
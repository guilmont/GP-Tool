#include "trajPlugin.h"

TrajPlugin::TrajPlugin(const Movie *mov, GPTool *ptr) : movie(mov), tool(ptr)
{
    const uint32_t SC = mov->getMetadata().SizeC;
    trackInfo.path.resize(SC);
}

TrajPlugin::~TrajPlugin(void) {}

void TrajPlugin::showWindows(void)
{
    if (trackInfo.show)
        winLoadTracks();
}

void TrajPlugin::showProperties(void)
{
    ImGui::Begin("Properties");

    if (m_traj)
    {
        float widthAvail = ImGui::GetContentRegionAvailWidth();
        ImGui::Text("Spot size: ");
        ImGui::SameLine();
        ImGui::PushItemWidth(0.5f * widthAvail);
        ImGui::SliderInt("##spotsize", &(trackInfo.spotSize), 2, 10);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Re-load"))
            enhanceTracks();

        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();

        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("TrajTabBar", tab_bar_flags))
        {
            const uint32_t nChannels = movie->getMetadata().SizeC;
            for (uint32_t ch = 0; ch < nChannels; ch++)
            {
                String txt = "Channel " + std::to_string(ch);
                if (ImGui::BeginTabItem(txt.c_str()))
                {
                    ImGui::PushID(txt.c_str());

                    Track track = m_traj->getTrack(ch);

                    String filename = track.path;
                    size_t pos = filename.find_last_of("/");
                    filename = filename.substr(pos + 1);

                    tool->fonts.text("Filename: ", "bold");
                    ImGui::SameLine();
                    ImGui::Text("%s", filename.c_str());

                    tool->fonts.text("Description: ", "bold");
                    ImGui::SameLine();
                    ImGui::Text("%s", track.description.c_str());

                    ImGui::Spacing();
                    ImGui::Spacing();

                    // // std::vector<View> &view = data.track->getTrack(ch).view;
                    // // ImGui::Text("Select: ");
                    // // ImGui::SameLine();
                    // // if (ImGui::Button("All"))
                    // // {
                    // //     for (uint32_t k = 0; k < view.size(); k++)
                    // //         view[k].first = true;

                    // //     data.updateViewport_flag = true;
                    // // }

                    // // ImGui::SameLine();
                    // // if (ImGui::Button("None"))
                    // // {
                    // //     for (uint32_t k = 0; k < view.size(); k++)
                    // //         view[k].first = false;

                    // //     data.updateViewport_flag = true;
                    // // }

                    // // ImGui::Spacing();

                    // // ImGui::Columns(2);
                    // // for (uint32_t k = 0; k < view.size(); k++)
                    // // {
                    // //     auto &[show, cor] = view.at(k);
                    // //     txt = "Trajectory " + std::to_string(k);
                    // //     ImGui::PushID(txt.c_str());

                    // //     if (ImGui::Checkbox("##check", &show))
                    // //         data.updateViewport_flag = true;

                    // //     ImGui::SameLine();
                    // //     ImGui::PushStyleColor(ImGuiCol_Text, {cor[0], cor[1], cor[2], 1.0});
                    // //     ui.fonts.text(txt, "bold");
                    // //     ImGui::PopStyleColor();
                    // //     ImGui::NextColumn();

                    // //     if (ImGui::Button("Detail"))
                    // //     {
                    // //         winTrajectory *win = (winTrajectory *)ui.mWindows["trajectory"].get();
                    // //         win->setInfo(ch, k);
                    // //         win->show();
                    // //     }

                    // //     ImGui::NextColumn();

                    //     ImGui::PopID();
                    // }

                    // ImGui::Columns(1);

                    ImGui::PopID();
                    ImGui::EndTabItem();
                } // if-tab

            } // loop-channels

            ImGui::EndTabBar();
        } // if-tab
    }
    else
    {
        ImGui::Text("No trajectories loaded!!");
        if (ImGui::Button("Load"))
            trackInfo.show = true;
    }

    ImGui::End();
}

void TrajPlugin::update(float deltaTime)
{
}

////////////////////////////////////////////////////////////////////////.//////
// PRIVATE FUNCTIONS

void TrajPlugin::winLoadTracks(void)
{
    ImGui::Begin("Tracks");

    ImGui::Text("Spot size: ");
    ImGui::SameLine();
    ImGui::SliderInt("##spotsize", &(trackInfo.spotSize), 2, 10);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    const uint32_t SC = movie->getMetadata().SizeC;
    float width = 0.75f * ImGui::GetContentRegionAvailWidth();

    for (uint32_t ch = 0; ch < SC; ch++)
    {
        std::string txt = "Channel " + std::to_string(ch);
        ImGui::PushID(txt.c_str());

        char buf[1024] = {0};
        ImGui::Text("%s", txt.c_str());
        ImGui::SameLine();

        strcpy(buf, trackInfo.path[ch].c_str());

        ImGui::SetNextItemWidth(width);
        ImGui::InputText("##input", buf, 1024, ImGuiInputTextFlags_ReadOnly);

        ImGui::SameLine();
        if (ImGui::Button("Open"))
        {
            trackInfo.openCH = ch;
            tool->dialog.createDialog(
                GDialog::OPEN, "Choose track...", {".xml", ".csv"}, this,
                [](void *ptr) -> void {
                    TrajPlugin *traj = (TrajPlugin *)ptr;

                    uint32_t ch = traj->trackInfo.openCH;
                    const std::string &path = traj->tool->dialog.getPath();
                    traj->trackInfo.path[ch] = path;
                });
        }

        ImGui::PopID();

        ImGui::Spacing();
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Load"))
    {
        enhanceTracks();
        trackInfo.show = false;

    } // if-load

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
        trackInfo.show = false;

    ImGui::End();
}

void TrajPlugin::enhanceTracks(void)
{
    m_traj = std::make_unique<Trajectory>(movie, &(tool->mbox));

    m_traj->spotSize = trackInfo.spotSize;
    for (uint32_t ch = 0; ch < trackInfo.path.size(); ch++)
    {
        std::string &path = trackInfo.path[ch];

        size_t pos = path.find_last_of('.');
        std::string ext = path.substr(pos + 1);

        if (ext.compare("xml") == 0)
            m_traj->useICY(path, ch);
        else if (ext.compare("csv") == 0)
            m_traj->useCSV(path, ch);

    } // loop - channels

    tool->mbox.create<Message::Info>("Optimizing trajectories...");
    std::thread([&](void) { m_traj->enhanceTracks(); }).detach();
}
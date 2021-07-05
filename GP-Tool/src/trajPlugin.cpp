#include "trajPlugin.h"

static void saveCSV(const std::string &path, const std::string *header, const MatXd &mat)
{

    const uint32_t nCols = uint32_t(mat.cols()),
                   nRows = uint32_t(mat.rows());

    std::ofstream arq(path);

    // Header
    for (uint32_t l = 0; l < nCols; l++)
        arq << header[l] << (l == nCols - 1 ? "\n" : ", ");

    // Body
    for (uint32_t k = 0; k < nRows; k++)
        for (uint32_t l = 0; l < nCols; l++)
            arq << mat(k, l) << (l == nCols - 1 ? "\n" : ", ");

    arq.close();

} // saveCSV

static Json::Value jsonEigen(const MatXd &mat)
{
    Json::Value array(Json::arrayValue);
    for (uint32_t k = 0; k < uint32_t(mat.cols()); k++)
        array.append(std::move(jsonArray(mat.col(k).data(), mat.rows())));

    return array;
}

///////////////////////////////////////////////////////////////////////////////

TrajPlugin::TrajPlugin(Movie *mov, GPTool *ptr) : movie(mov), tool(ptr)
{
    const uint32_t SC = mov->getMetadata().SizeC;
    trackInfo.path.resize(SC);

    uitraj = new UITraj[SC];
}

TrajPlugin::~TrajPlugin(void) { delete[] uitraj; }

void TrajPlugin::showWindows(void)
{
    if (trackInfo.show)
        winLoadTracks();

    if (detail.show)
        winDetail();

    if (plot.show)
        winPlots();
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
                std::string txt = "Channel " + std::to_string(ch);
                if (ImGui::BeginTabItem(txt.c_str()))
                {
                    ImGui::PushID(txt.c_str());

                    Track track = m_traj->getTrack(ch);

                    tool->fonts.text("Filename: ", "bold");
                    ImGui::SameLine();
                    ImGui::TextUnformatted(track.path.filename().string().c_str());

                    tool->fonts.text("Description: ", "bold");
                    ImGui::SameLine();
                    ImGui::TextUnformatted(track.description.c_str());

                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::Text("Select: ");
                    ImGui::SameLine();
                    if (ImGui::Button("All"))
                        for (auto &[show, cor] : uitraj[ch])
                            show = true;

                    ImGui::SameLine();
                    if (ImGui::Button("None"))
                        for (auto &[show, cor] : uitraj[ch])
                            show = false;

                    ImGui::Spacing();
                    ImGui::Columns(2);

                    for (uint32_t k = 0; k < uitraj[ch].size(); k++)
                    {
                        auto &[show, cor] = uitraj[ch].at(k);

                        txt = "Trajectory " + std::to_string(k);
                        ImGui::PushID(txt.c_str());

                        ImGui::Checkbox("##check", &show);

                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, {cor.r, cor.g, cor.b, 1.0f});
                        tool->fonts.text(txt, "bold");
                        ImGui::PopStyleColor();
                        ImGui::NextColumn();

                        if (ImGui::Button("Detail"))
                            detail = {true, ch, k};

                        ImGui::NextColumn();
                        ImGui::PopID();
                    } // loop-traj

                    ImGui::Columns(1);

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
    if (m_traj == nullptr)
    {
        tool->shader.setInteger("u_nPoints", 0);
        return;
    }

    MoviePlugin *mov = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"));

    const Metadata &meta = movie->getMetadata();

    const uint32_t frame = mov->current_frame,
                   nChannels = movie->getMetadata().SizeC;

    const glm::vec2 size = {float(meta.SizeY), float(meta.SizeX)};
    glm::mat3 trf(1.0f);

    uint32_t nPts = 0;
    std::array<glm::vec3, 128> vCor, vPos;
    for (uint32_t ch = 0; ch < nChannels; ch++)
    {
        int32_t ct = -1;
        const Track &track = m_traj->getTrack(ch);

        if (ch > 0)
        {
            AlignPlugin *al = reinterpret_cast<AlignPlugin *>(tool->getPlugin("ALIGNMENT"));
            trf = al->data[ch].trf;
        }

        for (auto &[show, cor] : uitraj[ch])
        {
            ct++;
            if (!show)
                continue;

            const MatXd &mat = track.traj.at(ct);
            for (uint32_t k = 0; k < uint32_t(mat.rows()); k++)
                if (uint32_t(mat(k, 0)) == frame)
                {
                    float x = float(mat(k, Track::POSX)),
                          y = float(mat(k, Track::POSY)),
                          rx = float(mat(k, Track::SIZEX)) / size.x,
                          ry = float(mat(k, Track::SIZEY)) / size.y;

                    float r = 0.5f * std::sqrt(rx * rx + ry * ry);

                    float nx = trf[0][0] * x + trf[0][1] * y + trf[0][2],
                          ny = trf[1][0] * x + trf[1][1] * y + trf[1][2];

                    vPos[nPts] = {nx, ny, r};
                    vCor[nPts] = cor;

                    if (++nPts == uint32_t(vPos.size()))
                        goto excess;

                    continue;
                }

        } // loop-traj

    } // loop-tracks

excess:

    // TODO: Use uniform block buffers
    tool->shader.setInteger("u_nPoints", nPts);
    tool->shader.setVec3fArray("u_ptPos", &vPos[0][0], nPts);
    tool->shader.setVec3fArray("u_ptColor", &vCor[0][0], nPts);

} // update

bool TrajPlugin::saveJSON(Json::Value &json)
{
    if (m_traj == nullptr)
        return false;

    const Metadata &meta = movie->getMetadata();

    json["PhysicalSizeXY"] = meta.PhysicalSizeXY;
    json["PhysicalSizeXYUnit"] = meta.PhysicalSizeXYUnit;
    json["TimeIncrementUnit"] = meta.TimeIncrementUnit;
    json["rows"] = "{frame, time, pos_x, error_x, pos_y, error_y,"
                   "size_x, size_y, background, signal}";

    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        const std::string ch_name = "channel_" + std::to_string(ch);
        const std::vector<MatXd> &vTraj = m_traj->getTrack(ch).traj;

        for (size_t k = 0; k < vTraj.size(); k++)
            json[ch_name]["traj_" + std::to_string(k)] = std::move(jsonEigen(vTraj[k]));

    } // loop-channels

    return true;

} //saveJSON

////////////////////////////////////////////////////////////////////////.//////
// PRIVATE FUNCTIONS

void TrajPlugin::enhanceTracks(void)
{
    m_traj = std::make_unique<Trajectory>(movie);

    m_traj->spotSize = trackInfo.spotSize;
    const uint32_t nChannels = movie->getMetadata().SizeC;

    for (uint32_t ch = 0; ch < nChannels; ch++)
    {
        const fs::path &path = trackInfo.path[ch];
        if (path.empty())
            continue;

        std::string ext = path.extension().string();
        if (ext.compare(".xml") == 0)
            m_traj->useICY(path.string(), ch);
        else if (ext.compare(".csv") == 0)
            m_traj->useCSV(path.string(), ch);

    } // loop - channels

    auto prog = tool->mbox.createProgress("Optimizing trajectories...", [](void *traj) { reinterpret_cast<Trajectory* >(traj)->stop(); }, m_traj.get());

    std::thread([](Trajectory* traj, Message::Progress* prog) {
        while (prog->progress < 1.0f)
            prog->progress = traj->getProgress();
        }, m_traj.get(), prog).detach();

    std::thread(
        [](Trajectory *m_traj, UITraj *uitraj, uint32_t nChannels)
        {
            m_traj->enhanceTracks();

            std::random_device rng;
            std::default_random_engine eng(rng());
            std::uniform_real_distribution<float> unif(0.0f, 1.0f);

            for (uint32_t ch = 0; ch < nChannels; ch++)
            {
                const Track &track = m_traj->getTrack(ch);
                const uint32_t NT = uint32_t(track.traj.size());

                uitraj[ch].resize(NT);
                for (uint32_t t = 0; t < NT; t++)
                    uitraj[ch].at(t) = {true, {unif(eng), unif(eng), unif(eng)}};

            } // loop-channels
        },
        m_traj.get(), uitraj, nChannels)
        .detach();

} // enhanceTracks

///////////////////////////////////////////////////////////////////////////////
// Windows

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
        ImGui::TextUnformatted(txt.c_str());
        ImGui::SameLine();

        strcpy(buf, trackInfo.path[ch].string().c_str());

        ImGui::SetNextItemWidth(width);
        ImGui::InputText("##input", buf, 1024, ImGuiInputTextFlags_ReadOnly);

        ImGui::SameLine();
        if (ImGui::Button("Open"))
        {
            trackInfo.openCH = ch;
            tool->dialog.createDialog(
                GDialog::OPEN, "Choose track...", {"xml", "csv"}, this,
                [](const std::string &path, void *ptr) -> void
                {
                    TrajPlugin *traj = (TrajPlugin *)ptr;

                    uint32_t ch = traj->trackInfo.openCH;
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

void TrajPlugin::winDetail(void)
{
    const Track &track = m_traj->getTrack(detail.trackID);
    const MatXd &mat = track.traj.at(detail.trajID);
    const uint32_t nRows = static_cast<uint32_t>(mat.rows());

    ImGui::Begin("Trajectory", &(detail.show));

    tool->fonts.text("Path: ", "bold");
    ImGui::SameLine();
    ImGui::TextUnformatted((const char*) track.path.c_str());

    tool->fonts.text("Description: ", "bold");
    ImGui::SameLine();
    ImGui::TextUnformatted(track.description.c_str());

    tool->fonts.text("Channel: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", detail.trackID);

    tool->fonts.text("Trajectory: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", detail.trajID);

    tool->fonts.text("Length: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%ld points", nRows);

    ImGui::Spacing();

    if (ImGui::Button("Export"))
    {
        const MatXd *mat = &(m_traj->getTrack(detail.trackID).traj[detail.trajID]);

        tool->dialog.createDialog(
            GDialog::SAVE, "Export CSV", {"csv"}, (void *)mat,
            [](const std::string &path, void *ptr) -> void
            {
                std::array<std::string, Track::NCOLS> header = {
                    "Frame", "Time", "Position X", "Position Y",
                    "Error X", "Error Y", "Size X", "Size Y",
                    "Background", "Signal"};

                MatXd &mat = *reinterpret_cast<MatXd *>(ptr);

                saveCSV(path, header.data(), mat);
            });
    }

    ImGui::SameLine();

    if (ImGui::Button("View plots"))
    {
        plot.show = true;
        plot.plotID = 0;
        plot.trackID = detail.trackID;
        plot.trajID = detail.trajID;
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // Creating table
    const char *names[] = {"Frame", "Time",
                           "Position X", "Position Y",
                           "Error X", "Error Y",
                           "Size X", "Size Y",
                           "Background", "Signal"};

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY;
    flags |= ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV;

    if (ImGui::BeginTable("table1", Track::NCOLS, flags))
    {
        // Header
        for (uint32_t k = 0; k < Track::NCOLS; k++)
            ImGui::TableSetupColumn(names[k]);
        ImGui::TableHeadersRow();

        // Mainbody
        for (uint32_t row = 0; row < nRows; row++)
        {
            ImGui::TableNextRow();

            //Frame column presents integer values
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.0f", mat(row, 0));

            // remains columns
            for (uint32_t column = 1; column < Track::NCOLS; column++)
            {
                ImGui::TableSetColumnIndex(column);
                ImGui::Text("%.3f", mat(row, column));
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();
} // winDetail

void TrajPlugin::winPlots(void)
{
    ImGui::Begin("Plots", &(plot.show));

    uint32_t &id = plot.plotID;
    if (ImGui::BeginCombo("Choose", plot.options[id]))
    {
        for (uint32_t k = 0; k < 3; k++)
            if (ImGui::Selectable(plot.options[k]))
            {
                id = k;
                ImGui::SetItemDefaultFocus();
            } // if-selected

        ImGui::EndCombo();
    }

    // Plot size
    const float avail = ImGui::GetContentRegionAvailWidth();
    const ImVec2 size = {0.99f * avail, 0.6f * avail};

    // Data to plot
    const MatXd &mat = m_traj->getTrack(plot.trackID).traj[plot.trajID];

    const uint32_t nPts = uint32_t(mat.rows());
    const double xmin = mat(0, 0),
                 xmax = mat(nPts - 1, 0);

    const VecXd &frame = mat.col(Track::FRAME);

    if (id == 0) // Movement
    {
        VecXd X = mat.col(Track::POSX).array() - mat(0, Track::POSX),
              Y = mat.col(Track::POSY).array() - mat(0, Track::POSY);

        VecXd errx = 1.96 * mat.col(Track::ERRX),
              erry = 1.96 * mat.col(Track::ERRY);

        VecXd lowX = X - errx, highX = X + errx;
        VecXd lowY = Y - erry, highY = Y + erry;

        const double ymin = std::min(lowX.minCoeff(), lowY.minCoeff()),
                     ymax = std::max(highX.maxCoeff(), highY.maxCoeff());

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax);

        if (ImPlot::BeginPlot("Movement", "Frame", "Position", size))
        {
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
            ImPlot::PlotShaded("X-Axis", frame.data(), lowX.data(), highX.data(), nPts);
            ImPlot::PlotLine("X-Axis", frame.data(), X.data(), nPts);

            ImPlot::PlotShaded("Y-Axis", frame.data(), lowY.data(), highY.data(), nPts);
            ImPlot::PlotLine("Y-Axis", frame.data(), Y.data(), nPts);
            ImPlot::PopStyleVar();

            ImPlot::EndPlot();
        }
    } // if-movement

    else if (id == 1) // Spot size
    {
        const VecXd &szx = mat.col(Track::SIZEX),
                    &szy = mat.col(Track::SIZEY);

        const double ymin = 0.5 * std::min(szx.minCoeff(), szy.minCoeff()),
                     ymax = 1.2 * std::max(szx.maxCoeff(), szy.maxCoeff());

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax);
        if (ImPlot::BeginPlot("Spot size", "Frame", "Size", size))
        {
            ImPlot::PlotLine("X-Axis", frame.data(), szx.data(), nPts);
            ImPlot::PlotLine("Y-Axis", frame.data(), szy.data(), nPts);
            ImPlot::EndPlot();
        }
    } // if-size

    else // Signal and background
    {
        const VecXd &bg = mat.col(Track::BG),
                    &sig = mat.col(Track::SIGNAL);

        const double ymin = 0.5 * std::min(bg.minCoeff(), sig.minCoeff()),
                     ymax = 1.2 * std::max(bg.maxCoeff(), sig.maxCoeff());

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax);
        if (ImPlot::BeginPlot("Signal", "Frame", "Signal", size))
        {
            ImPlot::PlotLine("Spot", mat.col(0).data(), sig.data(), nPts);
            ImPlot::PlotLine("Background", mat.col(0).data(), bg.data(), nPts);
            ImPlot::EndPlot();
        }
    } // else-signal

    ImGui::End();
} // winPlots
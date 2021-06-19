#include "gpPlugin.h"

#include <thread>
#include <fstream>

static constexpr uint32_t sample_size = 10000;

static void saveCSV(const std::string &path, const std::string *header,
                    const MatXd &mat)
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

static void binOption(int &bins)
{

    ImGui::SetNextItemWidth(200);

    if (ImGui::RadioButton("Sqrt", bins == ImPlotBin_Sqrt))
        bins = ImPlotBin_Sqrt;

    ImGui::SameLine();
    if (ImGui::RadioButton("Sturges", bins == ImPlotBin_Sturges))
        bins = ImPlotBin_Sturges;

    ImGui::SameLine();
    if (ImGui::RadioButton("Rice", bins == ImPlotBin_Rice))
        bins = ImPlotBin_Rice;

    ImGui::SameLine();
    if (ImGui::RadioButton("Scott", bins == ImPlotBin_Scott))
        bins = ImPlotBin_Scott;

    ImGui::SameLine();
    if (ImGui::RadioButton("N Bins", bins >= 0))
        bins = 50;

    if (bins >= 0)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::SliderInt("##Bins", &bins, 1, 100);
    }

    ImGui::Spacing();
    ImGui::Spacing();
} // binOption

static Json::Value jsonEigen(const MatXd &mat)
{
    Json::Value array(Json::arrayValue);
    for (uint32_t k = 0; k < uint32_t(mat.cols()); k++)
        array.append(std::move(jsonArray(mat.col(k).data(), mat.rows())));

    return array;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GPPlugin::GPPlugin(GPTool *ptr) : tool(ptr) {}
GPPlugin::~GPPlugin(void) {}

void GPPlugin::showWindows(void)
{
    if (avgView.show)
        winAvgView();

    if (subView.show)
        winSubstrate();

    if (subPlotView.show)
        winPlotSubstrate();

    if (distribView.show)
        winDistributions();
}

void GPPlugin::showProperties(void)
{
    TrajPlugin *pgl = reinterpret_cast<TrajPlugin *>(tool->getPlugin("TRAJECTORY"));
    const Trajectory *vTrack = pgl->getTrajectory();

    ImGui::Begin("Properties");

    if (vTrack == nullptr)
    {
        ImGui::Text("Trajectories not loaded!!");
        ImGui::End();

        return;
    }

    ImGui::Text("- Infer parameters\n"
                "- Estimate distributitions\n"
                "- Expected trajectories\n"
                "- Substrate movement");

    ImGui::Spacing();

    UITraj *ui = pgl->getUITrajectory(); // tells which are active

    if (ImGui::Button("Add cell"))
    {
        // Checking which trajectories are selected
        std::vector<MatXd> vTraj;
        std::vector<GP_FBM::ParticleID> partID;

        const uint32_t nTracks = vTrack->getNumTracks();
        for (uint32_t tr = 0; tr < nTracks; tr++)
        {
            const Track &track = vTrack->getTrack(tr);
            const uint32_t nTraj = uint32_t(track.traj.size());

            // TODO: Correct trajectory for alignment when necessary
            uint32_t k = 0;
            for (auto &[on, cor] : ui[tr])
            {
                if (on)
                {
                    if (track.traj[k].rows() < 50)
                    {
                        tool->mbox.createWarn("Discarding trajectory with less than 50 frames!");
                    }
                    else
                    {
                        MatXd loc = track.traj[k].block(0, 0, track.traj[k].rows(), 6);
                        vTraj.emplace_back(loc);
                        partID.push_back({tr, k});
                    }
                }

                k++;
            } // loop-trajectories

        } // loop-tracks

        if (vTraj.size() == 0)
            tool->mbox.createWarn("No trajectories selected!!");

        else if (vTraj.size() > 5)
            tool->mbox.createWarn("A maximum of 5 trajectories are allowed per cell!");

        else
        {
            const uint32_t nTraj = uint32_t(vTraj.size());
            
            std::thread(&GPPlugin::addNewCell, this, vTraj, partID).detach();
       
        }
    } // if-add-cell

    ImGui::Spacing();
    ImGui::Spacing();

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_None;
    nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    nodeFlags |= ImGuiTreeNodeFlags_Framed;
    nodeFlags |= ImGuiTreeNodeFlags_FramePadding;
    nodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    nodeFlags |= ImGuiTreeNodeFlags_AllowItemOverlap;

    int32_t gpID = -1,
            toRemove = -1; // If we want to remove any cell

    for (auto &gp : vecGP)
    {
        const Movie *movie = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"))->getMovie();

        const float
            pix2mu = movie->getMetadata().PhysicalSizeXY,
            DCalib = pix2mu * pix2mu;

        const std::string
            &spaceUnit = movie->getMetadata().PhysicalSizeXYUnit,
            &timeUnit = movie->getMetadata().TimeIncrementUnit;

        std::array<std::string, 5> header = {"Channel", "TrajID", "D", "Alpha", "Avg traj"};

        header[2] += " (" + spaceUnit + "^2/" + timeUnit + "^A)";

        gpID++;

        //////////////
        std::string txt = "Cell " + std::to_string(gpID);
        bool openTree = ImGui::TreeNodeEx(txt.c_str(), nodeFlags);
        ImGui::PushID(txt.c_str());

        float fSize = ImGui::GetContentRegionAvailWidth() - 6 * ImGui::GetFontSize();
        ImGui::SameLine(fSize);

        if (ImGui::Button("Select"))
        {
            // Setting everything to false
            const uint32_t nTracks = uint32_t(vTrack->getNumTracks());
            for (uint32_t ch = 0; ch < nTracks; ch++)
                for (auto &[show, color] : ui[ch])
                    show = false;

            // Activating only important ones
            for (auto [ch, pt] : gp->partID)
                ui[ch][pt].first = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Remove"))
        {
            toRemove = gpID;

            if (avgView.gpID == gpID)
                avgView.show = false;

            if (subView.gpID == gpID)
                subView.show = false;

            if (subPlotView.gpID == gpID)
                subPlotView.show = false;
        }

        if (openTree)
        {
            const uint32_t nParticles = gp->getNumParticles();

            // Creating table

            ImGuiTableFlags flags = ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV;

            if (ImGui::BeginTable("table1", 5, flags))
            {
                // Header
                for (uint32_t k = 0; k < 5; k++)
                    ImGui::TableSetupColumn(header[k].c_str());
                ImGui::TableHeadersRow();

                // Mainbody
                for (uint32_t k = 0; k < nParticles; k++)
                {
                    std::string txt = "part" + std::to_string(k);
                    ImGui::PushID(txt.c_str());
                    ImGui::TableNextRow();

                    //Frame column presents integer values
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", gp->partID[k].trackID);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", gp->partID[k].trajID);

                    float D, A;
                    if (nParticles == 1)
                    {
                        D = float(gp->singleModel(k)->D);
                        A = float(gp->singleModel(k)->A);
                    }
                    else
                    {
                        D = float(gp->coupledModel()->da[k].D);
                        A = float(gp->coupledModel()->da[k].A);
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.2e", double(pix2mu) * double(D));
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.3f", A);

                    ImGui::TableSetColumnIndex(4);

                    if (ImGui::Button("View", {ImGui::GetContentRegionAvailWidth(), 0}))
                        avgView = {true, uint32_t(gpID), k};

                    ImGui::PopID();

                } // loop-particles

                ImGui::EndTable();
            }

            if (ImGui::Button("View distribution"))
            {
                distribView.gpID = uint32_t(gpID);

                std::thread([](GP_FBM *gp, Mailbox *mbox, bool *show) -> void
                            {
                                auto msg = mbox->createTimer("Calculating distributions ...", [](void *) {});

                                if (gp->getNumParticles() == 1)
                                    gp->distrib_singleModel(sample_size);
                                else
                                    gp->distrib_coupledModel(sample_size);

                                *show = true;
                                msg->stop();
                            },
                            vecGP[gpID].get(), &(tool->mbox), &(distribView.show))
                    .detach();

            } // button-distribution

            if (nParticles > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Substrate"))
                {
                    subView.gpID = uint32_t(gpID);

                    std::thread(
                        [](GP_FBM *gp, Mailbox *mbox, bool *show) -> void
                        {
                            auto msg = mbox->createTimer("Calculating substrate data...", [](void *) {});

                            gp->estimateSubstrateMovement();
                            gp->distrib_coupledModel(sample_size);
                            *show = true;

                            msg->stop();
                        },
                        vecGP[gpID].get(), &(tool->mbox), &(subView.show))
                        .detach();
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TreePop();
        } // if-treenode

        ImGui::PopID();
    } // loop-cells

    // Removing cell if needed
    if (toRemove >= 0)
        vecGP.erase(vecGP.begin() + toRemove);

    ImGui::End();

} // showProperties

void GPPlugin::update(float deltaTime) {}

bool GPPlugin::saveJSON(Json::Value &json)
{
    const uint32_t nCells = uint32_t(vecGP.size());

    if (nCells == 0)
        return false;

    MoviePlugin *pgl = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"));
    const Metadata &meta = pgl->getMovie()->getMetadata();
    const double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

    json["D_units"] = meta.PhysicalSizeXYUnit + "^2/" + meta.TimeIncrementUnit + "^A";

    for (uint32_t id = 0; id < nCells; id++)
    {
        Json::Value cell;
        GP_FBM *gp = vecGP[id].get();

        const uint32_t nParticles = gp->getNumParticles();

        // SINGLE MODEL
        Json::Value &single = cell["single"];
        single["columns"] = "channel, particle_id, D, A, mu_x, mu_y";

        single["dynamics"] = Json::arrayValue;
        for (uint32_t pt = 0; pt < nParticles; pt++)
        {
            GP_FBM::DA *da = gp->singleModel(pt);

            Json::Value row(Json::arrayValue);
            row.append(gp->partID[pt].trackID);
            row.append(gp->partID[pt].trajID);
            row.append(Dcalib * da->D);
            row.append(da->A);
            row.append(da->mu.x);
            row.append(da->mu.y);
            single["dynamics"].append(std::move(row));
        }

        if (nParticles > 1)
        {

            Json::Value &coupled = cell["coupled"];
            coupled["columns"] = "channel, particle_id, D, A";

            coupled["dynamics"] = Json::arrayValue;
            GP_FBM::CDA *cda = gp->coupledModel();
            for (uint32_t pt = 0; pt < nParticles; pt++)
            {
                Json::Value row(Json::arrayValue);
                row.append(gp->partID[pt].trackID);
                row.append(gp->partID[pt].trajID);
                row.append(Dcalib * cda->da[pt].D);
                row.append(cda->da[pt].A);
                coupled["dynamics"].append(std::move(row));
            }

            // Substrate
            coupled["substrate"]["D"] = Dcalib * cda->DR;
            coupled["substrate"]["A"] = cda->AR;

            const MatXd &mat = gp->estimateSubstrateMovement();

            coupled["substrate"]["trajectory"] = jsonEigen(mat);
            coupled["substrate"]["rows"] = "frame, time, pos_x, pos_y, error_x, error_y";
        }

        json["Cells"].append(std::move(cell));

    } // loop-cells

    return true;

} // saveJSON

///////////////////////////////////////////////////////////////////////////////
// WINDOWS ////////////////////////////////////////////////////////////////////

void GPPlugin::winAvgView(void)
{
    // Getting data from specific particle
    const GP_FBM::ParticleID pid = vecGP[avgView.gpID]->partID[avgView.trajID];

    const Trajectory *traj = reinterpret_cast<TrajPlugin *>(tool->getPlugin("TRAJECTORY"))->getTrajectory();

    // Getting its original trajectory
    const MatXd &orig = traj->getTrack(pid.trackID).traj[pid.trajID];
    const uint32_t nRows = uint32_t(orig.rows());

    // Preparing data for scatter plot
    VecXd OT = orig.col(Track::TIME),
          OX = orig.col(Track::POSX).array() - orig(0, Track::POSX),
          OY = orig.col(Track::POSY).array() - orig(0, Track::POSY);

    // Calculating most probable trajectory with error
    const uint32_t nPts = uint32_t(
        float(orig(nRows - 1, Track::TIME) - orig(0, Track::TIME)) / 0.05f);

    VecXd vt(nPts);
    for (uint32_t k = 0; k < nPts; k++)
        vt(k) = orig(0, Track::TIME) + 0.05f * k;

    const MatXd &mat = vecGP[avgView.gpID]->calcAvgTrajectory(vt, avgView.trajID);

    // Preparing data for shaded plot
    VecXd X = mat.col(Track::POSX - 1).array() - mat(0, Track::POSX - 1),
          Y = mat.col(Track::POSY - 1).array() - mat(0, Track::POSY - 1);

    VecXd errx = 1.96 * mat.col(Track::ERRX - 1),
          erry = 1.96 * mat.col(Track::ERRY - 1);

    VecXd lowX = X - errx, highX = X + errx;
    VecXd lowY = Y - erry, highY = Y + erry;

    const double ymin = std::min(lowX.minCoeff(), lowY.minCoeff()),
                 ymax = std::max(highX.maxCoeff(), highY.maxCoeff());

    // Setup title for the plot
    char buf[512] = {0};
    sprintf(buf, "Cell: %d -- Channel: %d -- ID: %d",
            avgView.gpID, pid.trackID, pid.trajID);

    ///////////////////////////////////////////////////////
    // Creating ImGui windows
    ImGui::Begin("Avg trajectory", &(avgView.show));

    const float avail = ImGui::GetContentRegionAvailWidth();
    const ImVec2 size = {0.99f * avail, 0.6f * avail};

    ImPlot::SetNextPlotLimits(vt(0), vt(nPts - 1), ymin, ymax);

    if (ImPlot::BeginPlot(buf, "Time", "Position", size))
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
        ImPlot::PlotShaded("Average X", vt.data(), lowX.data(), highX.data(), nPts);
        ImPlot::PlotLine("Average X", vt.data(), X.data(), nPts);

        ImPlot::PlotScatter("Measured X", OT.data(), OX.data(), uint32_t(OT.size()));

        ImPlot::PlotShaded("Average Y", vt.data(), lowY.data(), highY.data(), nPts);
        ImPlot::PlotLine("Average Y", vt.data(), Y.data(), nPts);
        ImPlot::PlotScatter(" Measured Y", OT.data(), OY.data(), uint32_t(OT.size()));

        ImPlot::PopStyleVar();

        ImPlot::EndPlot();
    }

    ImGui::End();
} // winAvgView

void GPPlugin::winSubstrate(void)
{
    // Gathering some information
    const Movie *movie = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"))->getMovie();

    const float
        pix2mu = movie->getMetadata().PhysicalSizeXY,
        DCalib = pix2mu * pix2mu;

    const char
        *spaceUnit = movie->getMetadata().PhysicalSizeXYUnit.c_str(),
        *timeUnit = movie->getMetadata().TimeIncrementUnit.c_str();

    GP_FBM *gp = vecGP[subView.gpID].get();
    const MatXd &mat = gp->estimateSubstrateMovement();
    const MatXd &distrib = gp->distrib_coupledModel();

    const uint32_t nRows = uint32_t(mat.rows()),
                   nParticles = gp->getNumParticles();

    // Proceeding to the window
    ImGui::Begin("Substrate", &(subView.show));

    tool->fonts.text("Cell " + std::to_string(subView.gpID) + ":", "bold");
    char buf[512] = {0};

    ImGui::Indent();
    sprintf(buf, "DR = %.3e %s^2/%s^A", DCalib * gp->coupledModel()->DR,
            spaceUnit, timeUnit);
    ImGui::Text(buf);

    memset(buf, 0, 512);
    sprintf(buf, "AR = %.3f", gp->coupledModel()->AR);
    ImGui::Text(buf);
    ImGui::Unindent();

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("SUBSTRATE DISTRIBUTIONS");
    ImGui::Spacing();

    static int bins = 50;
    binOption(bins);

    float width = 0.495f * ImGui::GetContentRegionAvailWidth();

    if (ImPlot::BeginPlot("##Histogram_diffusion", "Diffusion coefficient", "Density",
                          {width, 0.6f * width}))
    {
        memset(buf, 0, 512);
        sprintf(buf, "DR / %.4f %s^2", DCalib, spaceUnit);

        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        ImPlot::PlotHistogram(buf, distrib.col(2 * nParticles).data(),
                              10000, bins, false, true);
        ImPlot::EndPlot();
    }

    ImGui::SameLine();

    if (ImPlot::BeginPlot("##Histogram_alpha", "Anomalous coefficient", "Density",
                          {width, 0.6f * width}))
    {
        ImPlot::SetNextFillStyle({0.85f, 0.25f, 0.18f, 0.8f}, 0.5f);
        ImPlot::PlotHistogram("Alpha", distrib.col(2 * nParticles + 1).data(),
                              10000, bins, false, true);
        ImPlot::EndPlot();
    }

    ImGui::Spacing();

    ImGui::Separator();

    ImGui::Text("SUBSTRATE MOVEMENT");
    ImGui::SameLine();

    if (ImGui::Button("View plot"))
        subPlotView = {true, subView.gpID};

    ImGui::SameLine();
    if (ImGui::Button("Export"))
    {
        tool->dialog.createDialog(
            GDialog::SAVE, "Export CSV", {".csv"}, (void *)&mat,
            [](const std::string &path, void *ptr) -> void
            {
                const MatXd &mat = *reinterpret_cast<MatXd *>(ptr);

                std::array<std::string, 6> header = {"Frame", "Time",
                                                     "Position X", "Position Y",
                                                     "Error X", "Error Y"};
                saveCSV(path, header.data(), mat);
            });
    }

    ///////////////////////////////////////////////////////
    // Creating table
    const char *names[] = {"Frame", "Time",
                           "Position X", "Position Y",
                           "Error X", "Error Y"};

    ImGuiTableFlags flags = ImGuiTableFlags_ScrollY;
    flags |= ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV;

    if (ImGui::BeginTable("table1", 6, flags))
    {
        // Header
        for (uint32_t k = 0; k < 6; k++)
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
            for (uint32_t column = 1; column < 6; column++)
            {
                ImGui::TableSetColumnIndex(column);
                ImGui::Text("%.3f", mat(row, column));
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();

} // winSubstrate

void GPPlugin::winPlotSubstrate(void)
{
    // Getting its original trajectory
    const MatXd &mat = vecGP[subPlotView.gpID]->estimateSubstrateMovement();
    const uint32_t nPts = uint32_t(mat.rows());

    // Preparing data for shaded plot
    VecXd T = mat.col(Track::TIME),
          X = mat.col(Track::POSX).array() - mat(0, Track::POSX),
          Y = mat.col(Track::POSY).array() - mat(0, Track::POSY);

    VecXd errx = 1.96 * mat.col(Track::ERRX),
          erry = 1.96 * mat.col(Track::ERRY);

    VecXd lowX = X - errx, highX = X + errx;
    VecXd lowY = Y - erry, highY = Y + erry;

    const double ymin = std::min(lowX.minCoeff(), lowY.minCoeff()),
                 ymax = std::max(highX.maxCoeff(), highY.maxCoeff());

    // Setup title for the plot
    char buf[128] = {0};
    sprintf(buf, "Cell: %d", subPlotView.gpID);

    ///////////////////////////////////////////////////////
    // Creating ImGui windows
    ImGui::Begin("Substrate movement", &(subPlotView.show));

    const float avail = ImGui::GetContentRegionAvailWidth();
    const ImVec2 size = {0.99f * avail, 0.6f * avail};

    ImPlot::SetNextPlotLimits(T(0), T(nPts - 1), ymin, ymax);

    if (ImPlot::BeginPlot(buf, "Time", "Position", size))
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
        ImPlot::PlotShaded("X", T.data(), lowX.data(), highX.data(), nPts);
        ImPlot::PlotLine("X", T.data(), X.data(), nPts);

        ImPlot::PlotShaded("Y", T.data(), lowY.data(), highY.data(), nPts);
        ImPlot::PlotLine("Y", T.data(), Y.data(), nPts);
        ImPlot::PopStyleVar();

        ImPlot::EndPlot();
    }

    ImGui::End();
} // winSubstrate

void GPPlugin::winDistributions(void)
{
    ImGui::Begin("Distributions", &(distribView.show));

    static int bins = 50;
    binOption(bins); // helper function

    // Gathering the data we need
    GP_FBM *gp = vecGP[distribView.gpID].get();
    const uint32_t nParticles = gp->getNumParticles();

    const Metadata &meta = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"))->getMovie()->getMetadata();

    float Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

    const MatXd *mat = nullptr;
    if (nParticles > 1)
        mat = &(gp->distrib_coupledModel());
    else
        mat = &(gp->distrib_singleModel());

    float width = 0.495f * ImGui::GetContentRegionAvailWidth();

    for (uint32_t k = 0; k < nParticles; k++)
    {
        ImGui::Separator();

        char buf[512] = {0};
        sprintf(buf, "Channel %d :: ID %d ", gp->partID[k].trackID, gp->partID[k].trajID);

        ImGui::PushID(buf);
        ImGui::Text(buf);

        if (ImPlot::BeginPlot("##Histogram_diffusion", "Diffusion coefficient", "Density",
                              {width, 0.6f * width}))
        {
            memset(buf, 0, 512);
            sprintf(buf, "D / %.4f %s^2", Dcalib, meta.PhysicalSizeXYUnit.c_str());

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotHistogram(buf, mat->col(2 * k).data(), 10000, bins, false, true);
            ImPlot::EndPlot();
        }

        ImGui::SameLine();

        if (ImPlot::BeginPlot("##Histogram_alpha", "Anomalous coefficient", "Density",
                              {width, 0.6f * width}))
        {
            ImPlot::SetNextFillStyle({0.85f, 0.25f, 0.18f, 0.8f}, 0.5f);
            ImPlot::PlotHistogram("Alpha", mat->col(2 * k + 1).data(), 10000, bins, false, true);
            ImPlot::EndPlot();
        }
        ImGui::Spacing();
        ImGui::PopID();
    }
    ImGui::End();

} // winDistributions

void GPPlugin::addNewCell(const std::vector<MatXd>& vTraj, const std::vector<GP_FBM::ParticleID>& partID)
{

    GP_FBM* gp = new GP_FBM(vTraj);
    gp->partID = partID;

    auto msg = tool->mbox.createTimer("Optimizing cell's parameters", [](void* gp) { reinterpret_cast<GP_FBM*>(gp)->stop(); }, gp);

    if (vTraj.size() == 1)
    {
        if (!gp->singleModel())
        {
            tool->mbox.createWarn("GP-FBM :: single model didn't converge!");
            return;
        }
    }
    else
    {
        if (!gp->coupledModel())
        {
            tool->mbox.createWarn("GP-FBM :: coupled model didn't converge!");
            return;
        }
    }

    vecGP.emplace_back(std::move(gp));

    msg->stop();
}

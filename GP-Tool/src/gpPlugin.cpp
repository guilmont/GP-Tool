#include "gpPlugin.h"

#include <thread>
#include <fstream>

static constexpr uint64_t sample_size = 10000;

static void saveCSV(const fs::path &path, const std::string *header,
                    const MatXd &mat)
{

    const uint64_t nCols = uint64_t(mat.cols()),
                   nRows = uint64_t(mat.rows());

    std::ofstream arq(path);

    // Header
    for (uint64_t l = 0; l < nCols; l++)
        arq << header[l] << (l == nCols - 1 ? "\n" : ", ");

    // Body
    for (uint64_t k = 0; k < nRows; k++)
        for (uint64_t l = 0; l < nCols; l++)
            arq << mat(k, l) << (l == nCols - 1 ? "\n" : ", ");

    arq.close();

}

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
}

static Json::Value jsonEigen(const MatXd &mat)
{
    Json::Value array(Json::arrayValue);
    for (uint64_t k = 0; k < uint64_t(mat.cols()); k++)
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
    const GPT::Trajectory *vTrack = pgl->getTrajectory();

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
        std::vector<GPT::GP_FBM::ParticleID> partID;

        const uint64_t nTracks = vTrack->getNumTracks();
        for (uint64_t tr = 0; tr < nTracks; tr++)
        {
            const GPT::Track_API &track = vTrack->getTrack(tr);
            const uint64_t nTraj = track.traj.size();

            // TODO: Correct trajectory for alignment when necessary
            uint64_t k = 0;
            for (auto &[on, cor] : ui[tr])
            {
                if (on)
                {
                    if (track.traj[k].rows() < 50)
                    {
                        tool->mailbox.createWarn("Discarding trajectory with less than 50 frames!");
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
            tool->mailbox.createWarn("No trajectories selected!!");

        else if (vTraj.size() > 5)
            tool->mailbox.createWarn("A maximum of 5 trajectories are allowed per cell!");

        else
        {
            const uint64_t nTraj = vTraj.size();

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

    int64_t gpID = -1,
            toRemove = -1; // If we want to remove any cell

    for (auto &gshow : vecGP)
    {
        const GPT::Movie *movie = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"))->getMovie();

        const double
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
            const uint64_t nTracks = uint64_t(vTrack->getNumTracks());
            for (uint64_t ch = 0; ch < nTracks; ch++)
                for (auto &[show, color] : ui[ch])
                    show = false;

            // Activating only important ones
            for (auto [ch, pt] : gshow.gp->partID)
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
            const uint64_t nParticles = gshow.gp->getNumParticles();

            // Creating table

            ImGuiTableFlags flags = ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_BordersV;

            if (ImGui::BeginTable("table1", 5, flags))
            {
                // Header
                for (uint64_t k = 0; k < 5; k++)
                    ImGui::TableSetupColumn(header[k].c_str());
                ImGui::TableHeadersRow();

                // Mainbody
                for (uint64_t k = 0; k < nParticles; k++)
                {
                    std::string txt = "part" + std::to_string(k);
                    ImGui::PushID(txt.c_str());
                    ImGui::TableNextRow();

                    //Frame column presents integer values
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("%d", gshow.gp->partID[k].trackID);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%d", gshow.gp->partID[k].trajID);

                    double D, A;
                    if (nParticles == 1)
                    {
                        GPT::GP_FBM::DA* da = gshow.gp->singleModel(k);

                        D = DCalib * da->D;
                        A = da->A;
                    }
                    else
                    {
                        GPT::GP_FBM::CDA* cda = gshow.gp->coupledModel();

                        D = DCalib * cda->da[k].D;
                        A = cda->da[k].A;
                    }

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%.2e", D);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.3f", A);

                    ImGui::TableSetColumnIndex(4);

                    if (ImGui::Button("View", {ImGui::GetContentRegionAvailWidth(), 0}))
                        avgView = {true, static_cast<uint64_t>(gpID), k};

                    ImGui::PopID();

                } // loop-particles

                ImGui::EndTable();
            }

            if (ImGui::Button("View distribution"))
            {
                distribView.gpID = static_cast<uint64_t>(gpID);

                std::thread([](GP2Show *gshow, GRender::Mailbox *mailbox, bool *show) -> void
                {
                    auto msg = mailbox->createTimer("Calculating distributions ...", [](void *) {});

                    if (gshow->gp->getNumParticles() == 1)
                    {
                        if (!gshow->distribSingle)
                        {
                            MatXd distrib = gshow->gp->distrib_singleModel(sample_size);
                            gshow->distribSingle = std::make_unique<MatXd>(std::move(distrib));
                        }

                    }
                    else
                    {
                        if (!gshow->distribCouple)
                        {
                            MatXd distrib = gshow->gp->distrib_coupledModel(sample_size);
                            gshow->distribCouple = std::make_unique<MatXd>(std::move(distrib));
                        }

                    }

                    *show = true;
                    msg->stop();

                }, &vecGP[gpID], &(tool->mailbox), &(distribView.show)).detach();

            } // button-distribution

            if (nParticles > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Substrate"))
                {
                    subView.gpID = uint64_t(gpID);

                    std::thread(
                        [](GP2Show *gshow, GRender::Mailbox *mailbox, bool *show) -> void
                        {
                            auto msg = mailbox->createTimer("Calculating substrate data...", [](void *) {});

                            if (!gshow->substrate)
                            {
                                MatXd subs = gshow->gp->estimateSubstrateMovement();
                                gshow->substrate = std::make_unique<MatXd>(std::move(subs));
                            }

                            *show = true;
                            msg->stop();
                        },
                        &vecGP[gpID], &(tool->mailbox), &(subView.show))
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
    const uint64_t nCells = uint64_t(vecGP.size());

    if (nCells == 0)
        return false;

    MoviePlugin *pgl = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"));
    const GPT::Metadata &meta = pgl->getMovie()->getMetadata();
    const double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

    json["D_units"] = meta.PhysicalSizeXYUnit + "^2/" + meta.TimeIncrementUnit + "^A";

    for (uint64_t id = 0; id < nCells; id++)
    {
        Json::Value cell;
        GPT::GP_FBM *gp = vecGP[id].gp.get();

        const uint64_t nParticles = gp->getNumParticles();

        // SINGLE MODEL
        Json::Value &single = cell["single"];
        single["columns"] = "channel, particle_id, D, A, mu_x, mu_y";

        single["dynamics"] = Json::arrayValue;
        for (uint64_t pt = 0; pt < nParticles; pt++)
        {
            GPT::GP_FBM::DA *da = gp->singleModel(pt);

            Json::Value row(Json::arrayValue);
            row.append(gp->partID[pt].trackID);
            row.append(gp->partID[pt].trajID);
            row.append(Dcalib * da->D);
            row.append(da->A);
            row.append(da->mu(0));
            row.append(da->mu(1));
            single["dynamics"].append(std::move(row));
        }

        if (nParticles > 1)
        {

            Json::Value &coupled = cell["coupled"];
            coupled["columns"] = "channel, particle_id, D, A";

            coupled["dynamics"] = Json::arrayValue;
            GPT::GP_FBM::CDA *cda = gp->coupledModel();
            for (uint64_t pt = 0; pt < nParticles; pt++)
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
    const GPT::GP_FBM::ParticleID pid = vecGP[avgView.gpID].gp->partID[avgView.trajID];

    const GPT::Trajectory *traj = reinterpret_cast<TrajPlugin *>(tool->getPlugin("TRAJECTORY"))->getTrajectory();

    // Getting its original trajectory
    const MatXd &orig = traj->getTrack(pid.trackID).traj[pid.trajID];
    const uint64_t nRows = uint64_t(orig.rows());

    // Preparing data for scatter plot
    VecXd OT = orig.col(GPT::Track::TIME),
          OX = orig.col(GPT::Track::POSX).array() - orig(0, GPT::Track::POSX),
          OY = orig.col(GPT::Track::POSY).array() - orig(0, GPT::Track::POSY);

    // Calculating most probable trajectory with error
    GP2Show& gshow = vecGP[avgView.gpID];

    if (!gshow.average[avgView.trajID])
    {
        const uint64_t nPts = uint64_t(float(orig(nRows - 1, GPT::Track::TIME) - orig(0, GPT::Track::TIME)) / 0.05f);

        VecXd vt(nPts);
        for (uint64_t k = 0; k < nPts; k++)
            vt(k) = orig(0, GPT::Track::TIME) + 0.05 * k;

        MatXd avg = gshow.gp->calcAvgTrajectory(vt, avgView.trajID);
        gshow.average[avgView.trajID] = std::make_unique<MatXd>(std::move(avg));

    }

    const MatXd *mat = gshow.average[avgView.trajID].get();


    // Preparing data for shaded plot
    int64_t nPts = mat->rows();

    VecXd
        T = mat->col(GPT::Track::TIME),
        X = mat->col(GPT::Track::POSX).array() - (*mat)(0, GPT::Track::POSX),
        Y = mat->col(GPT::Track::POSY).array() - (*mat)(0, GPT::Track::POSY);

    VecXd errx = 1.96 * mat->col(GPT::Track::ERRX),
          erry = 1.96 * mat->col(GPT::Track::ERRY);

    VecXd lowX = X - errx, highX = X + errx;
    VecXd lowY = Y - erry, highY = Y + erry;

    const double ymin = std::min(lowX.minCoeff(), lowY.minCoeff()),
                 ymax = std::max(highX.maxCoeff(), highY.maxCoeff());

    // Setup title for the plot
    std::string buf = "Cell: " + std::to_string(avgView.gpID) + " -- Channel: " + std::to_string(pid.trackID) + " -- ID: " + std::to_string(pid.trajID);

    ///////////////////////////////////////////////////////
    // Creating ImGui windows
    ImGui::Begin("Avg trajectory", &(avgView.show));

    ImGui::SetWindowSize({ 890.0f * GRender::DPI_FACTOR, 500.0f * GRender::DPI_FACTOR });

    const ImVec2 avail = ImGui::GetContentRegionAvail();
    const ImVec2 size = {0.99f * avail.x, 0.99f * avail.y};

    ImPlot::SetNextPlotLimits(T(0), T(nPts - 1), ymin, ymax);

    if (ImPlot::BeginPlot(buf.c_str(), "Time", "Position", size))
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
        ImPlot::PlotShaded("Average X", T.data(), lowX.data(), highX.data(), static_cast<int32_t>(nPts));
        ImPlot::PlotLine("Average X", T.data(), X.data(), static_cast<int32_t>(nPts));

        ImPlot::PlotScatter("Measured X", OT.data(), OX.data(), static_cast<int32_t>(OT.size()));

        ImPlot::PlotShaded("Average Y", T.data(), lowY.data(), highY.data(), static_cast<int32_t>(nPts));
        ImPlot::PlotLine("Average Y", T.data(), Y.data(), static_cast<int32_t>(nPts));
        ImPlot::PlotScatter(" Measured Y", OT.data(), OY.data(), static_cast<int32_t>(OT.size()));

        ImPlot::PopStyleVar();

        ImPlot::EndPlot();
    }

    ImGui::End();
} // winAvgView

void GPPlugin::winSubstrate(void)
{
    // Gathering some information
    const GPT::Movie *movie = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"))->getMovie();

    const double
        pix2mu = movie->getMetadata().PhysicalSizeXY,
        DCalib = pix2mu * pix2mu;

    const char
        *spaceUnit = movie->getMetadata().PhysicalSizeXYUnit.c_str(),
        *timeUnit = movie->getMetadata().TimeIncrementUnit.c_str();

    GP2Show &gshow = vecGP[subView.gpID];
     
    const MatXd* mat = gshow.substrate.get();
    
    const uint64_t nRows = uint64_t(mat->rows()),
                   nParticles = gshow.gp->getNumParticles();

    // Proceeding to the window
    ImGui::Begin("Substrate", &(subView.show));
    ImGui::SetWindowSize({ 800.0f * GRender::DPI_FACTOR, 800.0f * GRender::DPI_FACTOR });

    tool->fonts.text("Cell " + std::to_string(subView.gpID) + ":", "bold");

    GPT::GP_FBM::CDA* cda = gshow.gp->coupledModel();

    ImGui::Indent();
    ImGui::Text("DR = %.3e %s^2/%s^A", DCalib * cda->DR, spaceUnit, timeUnit);
    ImGui::Text("AR = %.3f", cda->AR);
    ImGui::Unindent();

    ImGui::Separator();
    ImGui::Spacing();

    if (gshow.distribCouple)
    {
        ImGui::Text("SUBSTRATE DISTRIBUTIONS");
        ImGui::Spacing();

        static int bins = 50;
        binOption(bins);

        float width = 0.495f * ImGui::GetContentRegionAvailWidth();
        const MatXd* mat = gshow.distribCouple.get();

        if (ImPlot::BeginPlot("##Histogram_diffusion", "Apparent diffusion", "Density", { width, 0.6f * width }))
        {
            std::string buf = "DR / " + std::to_string(DCalib) + " " + spaceUnit + "^2";

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotHistogram(buf.c_str(), mat->col(2 * nParticles).data(), 10000, bins, false, true);
            ImPlot::EndPlot();
        }

        ImGui::SameLine();

        if (ImPlot::BeginPlot("##Histogram_alpha", "Anomalous coefficient", "Density", { width, 0.6f * width }))
        {
            ImPlot::SetNextFillStyle({ 0.85f, 0.25f, 0.18f, 0.8f }, 0.5f);
            ImPlot::PlotHistogram("Alpha", mat->col(2 * nParticles + 1).data(), 10000, bins, false, true);
            ImPlot::EndPlot();
        }

        ImGui::Spacing();
        ImGui::Separator();
    }

    ImGui::Text("SUBSTRATE MOVEMENT");
    ImGui::SameLine();

    if (ImGui::Button("View plot"))
        subPlotView = {true, subView.gpID};

    ImGui::SameLine();
    if (ImGui::Button("Export"))
    {
        tool->dialog.createDialog(
            GDialog::SAVE, "Export CSV", {".csv"}, (void *)&mat,
            [](const fs::path &path, void *ptr) -> void
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
        for (uint64_t k = 0; k < 6; k++)
            ImGui::TableSetupColumn(names[k]);
        ImGui::TableHeadersRow();

        // Mainbody
        for (uint64_t row = 0; row < nRows; row++)
        {
            ImGui::TableNextRow();

            //Frame column presents integer values
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%.0f", (*mat)(row, 0));

            // remains columns
            for (int32_t column = 1; column < 6; column++)
            {
                ImGui::TableSetColumnIndex(column);
                ImGui::Text("%.3f", (*mat)(row, column));
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void GPPlugin::winPlotSubstrate(void)
{
    // Getting its original trajectory
    GP2Show& gshow = vecGP[subPlotView.gpID];
    if (!gshow.substrate)
    {
        MatXd subs = gshow.gp->estimateSubstrateMovement();
        gshow.substrate = std::make_unique<MatXd>(std::move(subs));
    }

    const MatXd* mat = gshow.substrate.get();
    const uint64_t nPts = uint64_t(mat->rows());

    // Preparing data for shaded plot
    VecXd T = mat->col(GPT::Track::TIME),
          X = mat->col(GPT::Track::POSX).array() - (*mat)(0, GPT::Track::POSX),
          Y = mat->col(GPT::Track::POSY).array() - (*mat)(0, GPT::Track::POSY);

    VecXd errx = 1.96 * mat->col(GPT::Track::ERRX),
          erry = 1.96 * mat->col(GPT::Track::ERRY);

    VecXd lowX = X - errx, highX = X + errx;
    VecXd lowY = Y - erry, highY = Y + erry;

    const double ymin = std::min(lowX.minCoeff(), lowY.minCoeff()),
                 ymax = std::max(highX.maxCoeff(), highY.maxCoeff());

    // Setup title for the plot

    ///////////////////////////////////////////////////////
    // Creating ImGui windows
    ImGui::Begin("Substrate movement", &(subPlotView.show));
    ImGui::SetWindowSize({ 890.0f * GRender::DPI_FACTOR, 500.0f * GRender::DPI_FACTOR });

    const float avail = ImGui::GetContentRegionAvailWidth();
    const ImVec2 size = {0.99f * avail, 0.6f * avail};

    ImPlot::SetNextPlotLimits(T(0), T(nPts - 1), ymin, ymax);

    std::string buf = "Cell: " + std::to_string(subPlotView.gpID);
    if (ImPlot::BeginPlot(buf.c_str(), "Time", "Position", size))
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
        ImPlot::PlotShaded("X", T.data(), lowX.data(), highX.data(), static_cast<int32_t>(nPts));
        ImPlot::PlotLine("X", T.data(), X.data(), static_cast<int32_t>(nPts));

        ImPlot::PlotShaded("Y", T.data(), lowY.data(), highY.data(), static_cast<int32_t>(nPts));
        ImPlot::PlotLine("Y", T.data(), Y.data(), static_cast<int32_t>(nPts));
        ImPlot::PopStyleVar();

        ImPlot::EndPlot();
    }

    ImGui::End();
} 

void GPPlugin::winDistributions(void)
{
    ImGui::Begin("Distributions", &(distribView.show));
    ImGui::SetWindowSize({ 900.0f * GRender::DPI_FACTOR, 600.0f * GRender::DPI_FACTOR });

    static int bins = 50;
    binOption(bins); // helper function

    // Gathering the data we need
    GP2Show& gshow = vecGP[distribView.gpID];

    const uint64_t nParticles = gshow.gp->getNumParticles();

    const GPT::Metadata &meta = reinterpret_cast<MoviePlugin *>(tool->getPlugin("MOVIE"))->getMovie()->getMetadata();

    double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

    const MatXd *mat = nullptr;
    if (nParticles > 1)
        mat = gshow.distribCouple.get();
    else
        mat = gshow.distribSingle.get();

    float width = 0.495f * ImGui::GetContentRegionAvailWidth();

    for (uint64_t k = 0; k < nParticles; k++)
    {
        ImGui::Separator();

        std::string buf = "Channel " + std::to_string(gshow.gp->partID[k].trackID) + " ::ID " + std::to_string(gshow.gp->partID[k].trajID);

        ImGui::PushID(buf.c_str());
        ImGui::TextUnformatted(buf.c_str());

        if (ImPlot::BeginPlot("##Histogram_diffusion", "Apparent diffusion", "Density", {width, 0.6f * width}))
        {
            buf = "D / " + std::to_string(Dcalib) + " " + meta.PhysicalSizeXYUnit + "^2";

            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
            ImPlot::PlotHistogram(buf.c_str(), mat->col(2 * k).data(), 10000, bins, false, true);
            ImPlot::EndPlot();
        }

        ImGui::SameLine();

        if (ImPlot::BeginPlot("##Histogram_alpha", "Anomalous coefficient", "Density", {width, 0.6f * width}))
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

void GPPlugin::addNewCell(const std::vector<MatXd> &vTraj, const std::vector<GPT::GP_FBM::ParticleID> &partID)
{

    GP2Show loc;
    loc.gp = std::make_unique<GPT::GP_FBM>(vTraj);
    loc.gp->partID = partID;
    loc.average.resize(vTraj.size());
    
    auto msg = tool->mailbox.createTimer("Optimizing cell's parameters", [](void *gp) { reinterpret_cast<GPT::GP_FBM *>(gp)->stop(); }, loc.gp.get());

    if (vTraj.size() == 1)
    {
        if (!loc.gp->singleModel())
        {
            tool->mailbox.createWarn("GP-FBM :: single model didn't converge!");
            return;
        }
    }
    else
    {
        if (!loc.gp->coupledModel())
        {
            tool->mailbox.createWarn("GP-FBM :: coupled model didn't converge!");
            return;
        }
    }

    vecGP.emplace_back(std::move(loc));

    msg->stop();
}

#include "gpPlugin.h"

#include <thread>

GPPlugin::GPPlugin(GPTool *ptr) : tool(ptr) {}
GPPlugin::~GPPlugin(void) {}

void GPPlugin::showWindows(void) {}

void GPPlugin::showProperties(void)
{
    ImGui::Begin("Properties");

    ImGui::Text("- Infer parameters\n"
                "- Estimate distributitions\n"
                "- Expected trajectories\n"
                "- Substrate movement");

    ImGui::Spacing();

    //     auto addRow = [](uint32_t id, double D, double A, const GP_FBM *gp,
    //                      winPlotTrajectory *win) -> void {
    //         ImGui::Text("%d", id);
    //         ImGui::NextColumn();

    //         ImGui::Text("%.3e", D);
    //         ImGui::NextColumn();

    //         ImGui::Text("%.3f", A);
    //         ImGui::NextColumn();

    //         char txt[32];
    //         sprintf(txt, "Particle %d", id);
    //         ImGui::PushID(txt);

    //         float width = ImGui::GetContentRegionAvailWidth();
    //         if (ImGui::Button("View", {width, 0}))
    //         {
    //             win->setInfo(&gp->getAvgTrajectory(id), "Movement");
    //             win->show();
    //         }

    //         ImGui::PopID();
    //         ImGui::NextColumn();

    //         ImGui::Spacing();
    //     };

    if (ImGui::Button("Add cell"))
    {

        TrajPlugin *pgl = (TrajPlugin *)tool->manager->getPlugin("TRAJECTORY");
        const UITraj *ui = pgl->getUITrajectory();       // tells which are active
        const Trajectory *vTrack = pgl->getTrajectory(); //

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
                        const char *txt = "Discarding trajectory with "
                                          "less than 50 frames!";
                        tool->mbox.create<Message::Warn>(txt);
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
            tool->mbox.create<Message::Warn>("No trajectories selected!!");

        else if (vTraj.size() > 5)
            tool->mbox.create<Message::Warn>("A maximum of 5 trajectories are "
                                             "allowed per cell!");

        else
        {
            const uint32_t nTraj = uint32_t(vTraj.size());

            GP_FBM *gp = new GP_FBM(std::move(vTraj), &(tool->mbox));
            gp->partID = std::move(partID);
            vecGP.emplace_back(std::move(gp));

            auto runCell = [](uint32_t nTraj, GP_FBM *gp, Mailbox *mbox) -> void {
                Message::Timer *msg = nullptr;
                msg = mbox->create<Message::Timer>("Optimizing cell's parameters");

                if (nTraj == 1)
                {
                    if (!gp->singleModel())
                        return;
                }
                else
                {
                    if (!gp->coupledModel())
                        return;
                }

                msg->stop();
            };

            std::thread(runCell, nTraj, vecGP.back().get(), &(tool->mbox)).detach();
        }
    } // if-add-cell

    ImGui::Spacing();
    ImGui::Spacing();

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    const Movie *movie = ((MoviePlugin *)tool->manager->getPlugin("MOVIE"))->getMovie();

    const float
        pix2mu = movie->getMetadata().PhysicalSizeXY,
        DCalib = pix2mu * pix2mu;

    const char
        *spaceUnit = movie->getMetadata().PhysicalSizeXYUnit.c_str(),
        *timeUnit = movie->getMetadata().TimeIncrementUnit.c_str();

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_None;
    nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    nodeFlags |= ImGuiTreeNodeFlags_Framed;
    nodeFlags |= ImGuiTreeNodeFlags_FramePadding;
    nodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    nodeFlags |= ImGuiTreeNodeFlags_AllowItemOverlap;

    int32_t gpID = -1;
    for (auto &gp : vecGP)
    {
        gpID++;

        String txt = "Cell " + std::to_string(gpID);
        bool openTree = ImGui::TreeNodeEx(txt.c_str(), nodeFlags);

        float fSize = ImGui::GetContentRegionAvailWidth() - 6 * ImGui::GetFontSize();
        ImGui::SameLine(fSize);

        if (ImGui::Button("Select"))
        {
            // // Setting everything to false
            // for (uint32_t ch = 0; ch < data.track->getNumTracks(); ch++)
            //     for (auto &view : data.track->getTrack(ch).view)
            //         view.first = false;

            // // Activating only important ones
            // for (auto [ch, pt] : gp->partID)
            //     data.track->getTrack(ch).view[pt].first = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Remove"))
        {
            // ui.mWindows["substrate"]->hide();
            // ui.mWindows["plotDistrib"]->hide();

            // openTree = false;
            // vecGP.erase(data.vecGP.begin() + id);
        }

        if (openTree)
        {
            const uint32_t nParticles = gp->getNumParticles();

            // Creating fancy table
            ImGui::Separator();

            ImGui::Columns(4);
            ImGui::Text("ID");
            ImGui::NextColumn();
            ImGui::Text("D (%s^2/%s^A)", spaceUnit, timeUnit);
            ImGui::NextColumn();
            ImGui::Text("Alpha");
            ImGui::NextColumn();
            ImGui::Text("Avg Traj");
            ImGui::NextColumn();

            ImGui::Separator();

            // Table to display results

            // winPlotTrajectory *winPlot = (winPlotTrajectory *)ui.mWindows["plotTraj"].get();
            // if (nParticles == 1 && gp.hasSingle())
            // {
            //     const ParamDA &da = gp.getSingleParameters()[0];
            //     addRow(0, DCalib * da.D, da.A, &gp, winPlot);
            // } // if-single-particle

            // if (nParticles > 1 && gp.hasCoupled())
            // {
            //     const ParamCDA &cda = gp.getCoupledParameters();
            //     for (uint32_t pt = 0; pt < nParticles; pt++)
            //         addRow(pt, DCalib * cda.particle[pt].D, cda.particle[pt].A, &gp, winPlot);

            // } // if-multiple particles

            ImGui::Columns(1);
            ImGui::Separator();

            // float width = ImGui::GetContentRegionAvailWidth();
            if (ImGui::Button("View distribution"))
            {
                // winPlotDistribution *win = (winPlotDistribution *)ui.mWindows["plotDistrib"].get();
                // win->setCellID(id);
                // win->show();

                // GP_FBM *gp = &data.vecGP[id];
                // Mailbox *box = &ui.mail;

                // if (nParticles == 1 && !data.vecGP[id].hasDistribSingle())
                // {
                //     std::thread(runSingleDistrib, gp, box).detach();
                //     ui.mWindows["inbox"]->show();
                // }

                // if (nParticles > 1 && !data.vecGP[id].hasDistribCoupled())
                // {
                //     std::thread(runCoupledDistrib, gp, box).detach();
                //     ui.mWindows["inbox"]->show();
                // }

            } // button-distribution

            if (nParticles > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Substrate"))
                {
                    // winSubstrate *win = (winSubstrate *)ui.mWindows["substrate"].get();
                    // win->setCellID(id);
                    // win->show();

                    // if (!gp.hasSubstrate())
                    //     data.vecGP[id].estimateSubstrateMovement();

                } // button-substrate
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TreePop();
        } // if-treenode

    } // loop-cells

    ImGui::End();

} // showProperties

void GPPlugin::update(float deltaTime) {}
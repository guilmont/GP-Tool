#include "batch.h"
#include "gptool.h"

#include <set> // We will use it for interpolation

static Json::Value jsonEigen(const MatXd& mat)
{
    Json::Value array(Json::arrayValue);
    for (uint64_t k = 0; k < uint64_t(mat.cols()); k++)
        array.append(std::move(jsonArray(mat.col(k).data(), mat.rows())));

    return array;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Batching::Batching(GPTool* ptr) : tool(ptr) { suffices.resize(numChannels); }

void Batching::setActive(void) { view_batching = true; }

void Batching::imguiLayer(void)
{
    if (!view_batching)
        return;

    ImGui::SetNextWindowSize({ 600 * GRender::DPI_FACTOR, 400 * GRender::DPI_FACTOR }, ImGuiCond_Once);
    ImGui::Begin("Batching", &view_batching);

    float width = 0.6f * ImGui::GetContentRegionAvailWidth();
    float pos = 0.3f * ImGui::GetContentRegionAvailWidth();

    ImGui::PushID("mainPath");
    ImGui::Text("Main path:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::SetCursorPosX(pos);
    ImGui::InputText("##mainPath", mainPath, 1024);
    ImGui::SameLine();
    if (ImGui::Button("Browse"))
        tool->dialog.createDialog(GDialog::OPEN_DIRECTORY, "Batching :: main path", {}, mainPath, [](const fs::path& path, void* ptr) {
                std::memset(ptr, 0, 1024);
                const std::string& loc = path.string();
                std::copy(loc.c_str(), loc.c_str() + loc.size(), (char*)ptr);
            });
    ImGui::PopID();

    ImGui::PushID("outPath");
    ImGui::Text("Output path:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::SetCursorPosX(pos);
    ImGui::InputText("##outputPath", outPath, 1024);
    ImGui::SameLine();
    if (ImGui::Button("Browse"))
        tool->dialog.createDialog(GDialog::SAVE, "Batching :: output path", { "json" }, outPath, [](const fs::path& path, void* ptr) {
            std::memset(ptr, 0, 1024);
            const std::string& loc = path.string();
            std::copy(loc.c_str(), loc.c_str() + loc.size(), (char*)ptr);
        });
    ImGui::PopID();


    // Defining new width for drag bars
    width = 0.15f * ImGui::GetContentRegionAvailWidth();

    ImGui::Spacing();

    ImGui::Text("Number of channels:");
    ImGui::SameLine();
    ImGui::SetCursorPosX(pos);
    ImGui::SetNextItemWidth(width);
    if (ImGui::DragInt("##numChannels", &numChannels, 0.5f, 1, 5))
        suffices.resize(numChannels);

    ImGui::Text("Number of threads:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(width);
    ImGui::SetCursorPosX(pos);
    ImGui::DragInt("##numThreads", &numThreads, 0.5f, 1, std::thread::hardware_concurrency());

    ImGui::Dummy({ -1, 5 });
    tool->fonts.text("Plugins", "bold");

    runAlignment = false;
    if (numChannels > 1)
    {
        if (ImGui::TreeNode("Alignment"))
        {
            runAlignment = true;
            ImGui::RadioButton("Individual", &alignID, ALIGN::INDIVIDUAL);
            ImGui::RadioButton("Bundled", &alignID, ALIGN::BUNDLED);
            ImGui::Spacing();
            ImGui::Checkbox("Camera alignment", &checkCamera);
            ImGui::Checkbox("Correct aberrations", &checkAberration);
            ImGui::TreePop();
        }

        ImGui::Dummy({ -1, 10.0f });
    }


    runTrajectories = false;
    if (ImGui::TreeNode("Trajectories"))
    {
        runTrajectories = true;

        ImGui::Text("Spot size:");
        ImGui::SameLine();
        ImGui::SetCursorPosX(pos);
        ImGui::SetNextItemWidth(width);
        ImGui::DragInt("##spotsize", &spotSize, 0.5f, 3, 10);

      

        for (int32_t k = 0; k < numChannels; k++)
        {
            ImGui::PushID(k);
            ImGui::Text("Suffix :: Channel %d", k);
            ImGui::SameLine();
            ImGui::SetCursorPosX(pos);
            ImGui::InputText("##suffix", suffices[k].data(), 512);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    ImGui::Dummy({ -1, 10.0f });
    runGPFBM = false;
    if (ImGui::TreeNode("GP-FBM"))
    {
        runGPFBM = true;
        ImGui::Checkbox("Single analysis", &checkSingle);
        if (checkSingle)
        {
            ImGui::Dummy({ 2.0f, -1 }); ImGui::SameLine();
            ImGui::Checkbox("Interpolate trajectories", &checkInterpol);
        }

        ImGui::Checkbox("Substrate correction", &checkCoupled);

        if (checkCoupled)
        {
            ImGui::Dummy({ 2.0f, -1 }); ImGui::SameLine();
            ImGui::Checkbox("Estimate substrate movement", &checkSubstrate);
        }

        ImGui::TreePop();
    }

    ImGui::Dummy({ -1, 10.0f });

    if (ImGui::Button("Run"))
        std::thread(&Batching::run, this).detach(); // Submitting to another thread, so it doesn't block GP-Tool

    ImGui::SameLine();
    if (ImGui::Button("Close"))
    {
        // Reseting everything
        std::memset(mainPath, 0, 1024);
        std::memset(outPath, 0, 1024);

        view_batching = false;
        
        checkCamera = checkAberration = true;
        checkSingle = checkInterpol = false;
        checkCoupled = checkSubstrate = false;

        alignID = ALIGN::INDIVIDUAL;
        numChannels = 1;
        suffices.resize(numChannels);
    }


    ImGui::End();
}

void Batching::run(void)
{
    // Check if all the movies and tracks are in good state
    tool->mailbox.createInfo("Checking files...");

     // Checking if directories are fine
    if (!fs::exists(mainPath))
    {
        tool->mailbox.createError("Input directory doesn't exist: " + std::string(mainPath));
        return;
    }

    fs::path output(outPath);
    if (!fs::exists(output.parent_path()))
    {
        tool->mailbox.createError("Output directory doesn't exist: " + output.string());
        return;
    }


    // Recursively collecting data for all the tif files present in directory and subdirectories
    bool stopAnalysis = false;
    vecSamples.clear();

    for (fs::directory_entry entry : fs::recursive_directory_iterator(mainPath))
    {
        const fs::path& loc = entry.path();
        if (!fs::is_regular_file(loc) || loc.extension().string().compare(".tif") != 0)
            continue;


        // We have a tif file, but can we open it?
        GPT::Movie mov(loc);
        if (!mov.successful())
        {
            tool->mailbox.createError("Cannot open movie: " + loc.string());
            stopAnalysis = true;
            continue;
        }

        if (runAlignment && mov.getMetadata().SizeC == 1)
        {
            tool->mailbox.createError("Cannot align movie with single channel: " + loc.string());
            stopAnalysis = true;
            continue;
        }

        // Are we running trajectory enhancement
        if (!runTrajectories && !runAlignment)
            continue;

        // We can open the movie
        MovData data;
        data.moviePath = loc;

        // Let's check if the trajectories are fine
        if (runTrajectories)
        {
            GPT::Trajectory traj(&mov);
            for (int32_t ch = 0; ch < numChannels; ch++)
            {
                fs::path trajPath = loc.parent_path() / (loc.stem().string() + std::string(suffices[ch].data()));

                if (trajPath.extension().compare(".xml") == 0)
                {
                    if (!traj.useICY(trajPath, ch))
                    {
                        tool->mailbox.createError("Cannot open trajectory: " + trajPath.string());
                        stopAnalysis = true;
                        continue;
                    }
                }
                else if (trajPath.extension().compare(".csv") == 0)
                {
                    if (!traj.useCSV(trajPath, ch))
                    {
                        tool->mailbox.createError("Cannot open trajectory: " + trajPath.string());
                        stopAnalysis = true;
                        continue;
                    }
                }
                else
                {
                    tool->mailbox.createError("File extension not recognized: " + trajPath.string());
                    stopAnalysis = true;
                    continue;
                }

                // If it reached here, insert trajectory path to data
                data.trajPath.emplace_back(trajPath);
            }
        }

        if (!stopAnalysis)
            vecSamples.emplace_back(std::move(data));

    }

    // If there was a problem with any file, stop here
    if (stopAnalysis)
    {
        tool->mailbox.createInfo("Stopping batching analysis...");
        return;
    }

    if (vecSamples.empty())
    {
        tool->mailbox.createInfo("Batching has nothing to run...");
        return;
    }

    // If it reached here, we are good to run the analysis
    // Let's split into the set number of threads
    
    outputJson.clear(); // Just in case
    cancelBatch = false;
    
    // To avoid thread racing, let's create objects for all the movies
    for (const MovData& mov : vecSamples)
        outputJson["Movies"][mov.moviePath.stem().string()] = Json::objectValue;


    // Also, we need to alocate space for alignment if in bundled mode
    if (runAlignment && alignID == ALIGN::BUNDLED)
        vecImagesToAlign = std::vector<std::vector<MatXd>>(numChannels, std::vector<MatXd>(vecSamples.size())); // We will save the first image of each channel for every movie


    // Splitting into threads
    std::vector<std::thread> vThr(numThreads);
    for (int32_t k = 0; k < numThreads; k++)
        vThr[k] = std::thread(&Batching::runSamples, this, k);

    for (std::thread& thr : vThr)
        thr.join();

    if (cancelBatch)
    {
        tool->mailbox.createInfo("Batch was cancelled!");
        return;
    }

    if (runAlignment && (alignID == ALIGN::BUNDLED))
    {
        GRender::Timer* ptr = tool->mailbox.createTimer("Running alignment", [](void* ptr) { *reinterpret_cast<bool*>(ptr) = true; }, &cancelBatch);

        for (int32_t ch = 1; ch < numChannels; ch++)
        {
            GPT::Align var(2, vecImagesToAlign[0].data(), vecImagesToAlign[ch].data());

            if (checkCamera && !cancelBatch)
                var.alignCameras();

            if (checkAberration && !cancelBatch)
                var.correctAberrations();

            const GPT::TransformData& RT = var.getTransformData();

            Json::Value& jsonAlign = outputJson["Alignment"]["channel_" + std::to_string(ch)];
            jsonAlign["translate"] = jsonArray(RT.translate.data(), 2);
            jsonAlign["rotate"] = jsonArray(RT.rotate.data(), 3);
            jsonAlign["scale"] = jsonArray(RT.scale.data(), 2);
            jsonAlign["size"] = jsonArray(RT.size.data(), 2);
            jsonAlign["transform"] = jsonEigen(RT.trf.transpose());
            
            if (cancelBatch)
            {
                tool->mailbox.createInfo("Batch was cancelled!");
                return;
            }

            ptr->stop();
        }
    }

    // Saving results to file
    std::ofstream arq(outPath);
    arq << outputJson;
    arq.close();

    tool->mailbox.createInfo("Batching completed successfully");
}

void Batching::runSamples(const int32_t threadId)
{
    GRender::Progress* prog = nullptr;

    if (threadId == 0)
        prog = tool->mailbox.createProgress("Running samples", [](void* check) { *reinterpret_cast<bool*>(check) = true; }, &cancelBatch);

    
    for (int32_t id = threadId; id < vecSamples.size(); id += numThreads)
    {
        if (cancelBatch)
            return;
    
        const MovData& data = vecSamples[id];

        const std::string name = data.moviePath.stem().string();
        
        // Just getting a local reference to faciliate
        Json::Value& local = outputJson["Movies"][name];

        GPT::Movie mov(data.moviePath);
        const GPT::Metadata& meta = mov.getMetadata();
        
        // First let's handle the trajectory enhancement
        if (runTrajectories)
        {
            GPT::Trajectory traj(&mov);
            traj.spotSize = spotSize;

            for (uint64_t ch = 0; ch < numChannels; ch++)
            {
                if (data.trajPath[ch].extension().string().compare(".xml") == 0)
                    traj.useICY(data.trajPath[ch], ch);

                else if (data.trajPath[ch].extension().string().compare(".csv") == 0)
                    traj.useCSV(data.trajPath[ch], ch);
            }

            traj.enhanceTracks();


            // Saving some comments so we are not lost
            local["Trajectories"]["physicalSizeXY"] = meta.PhysicalSizeXY;
            local["Trajectories"]["physicalSizeXYUnit"] = meta.PhysicalSizeXYUnit;
            local["Trajectories"]["timeIncrementUnit"] = meta.TimeIncrementUnit;
            local["Trajectories"]["rows"] = "frame, time, pos_x, pos_y, error_x, error_y, size_x, size_y,background, signal";

            for (int32_t ch = 0; ch < numChannels; ch++)
            {
                Json::Value& jsonTraj = local["Trajectories"]["channel_" + std::to_string(ch)];

                const std::vector<MatXd>& vec = traj.getTrack(ch).traj;
                for (int32_t k = 0; k < vec.size(); k++)
                    jsonTraj["traj_" + std::to_string(k)] = jsonEigen(vec[k]);
            }

            // Let's do the GP-FBM section here
            if (runGPFBM)
            {
                // Let's create a vector with all the trajectories in this movie
                // We also create a id for each trajectory
                std::vector<MatXd> vTraj;
                std::vector<GPT::GP_FBM::ParticleID> vecID;

                for (uint64_t trId = 0; trId < traj.getNumTracks(); trId++)
                {
                    const GPT::Track_API& track = traj.getTrack(trId);
                    const uint64_t nTrajs = track.traj.size();

                    for (uint64_t k = 0; k < nTrajs; k++)
                    {
                        vTraj.push_back(track.traj[k]);
                        vecID.push_back({ trId, k });
                    }
                }

                Json::Value& jsonGP = local["GProcess"];
                jsonGP["D_units"] = meta.PhysicalSizeXYUnit + "^2/" + meta.TimeIncrementUnit + "^A";

                GPT::GP_FBM gp(vTraj);
                double CONV = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

                if (checkSingle)
                {
                    double loc[6];
                    jsonGP["Single"]["dynamics_columns"] = "channel, particle_id, D, A, mu_x, mu_y";
                    for (uint64_t k = 0; k < vTraj.size(); k++)
                    {
                        GPT::GP_FBM::DA* da = gp.singleModel(k);
                        loc[0] = double(vecID[k].trackID);
                        loc[1] = double(vecID[k].trajID);
                        loc[2] = CONV * da->D;
                        loc[3] = da->A;
                        loc[4] = da->mu[0];
                        loc[5] = da->mu[1];

                        jsonGP["Single"]["dynamics"].append(jsonArray(loc, 6));
                    }

                    if (checkInterpol)
                    {
                        // Get all all time points in which points where detected
                        std::set<double> vTime;
                        for (const MatXd& mat : vTraj)
                        {
                            const VecXd& vt = mat.col(GPT::Track::TIME);
                            vTime.insert(vt.data(), vt.data() + vt.size());
                        }

                        auto it = vTime.begin();
                        VecXd vt(vTime.size());

                        for (uint64_t k = 0; k < vTime.size(); k++)
                        {
                            vt(k) = *it;
                            it++;
                        }

                        jsonGP["Single"]["interpolation_columns"] = "time, pos_x, pos_y, error_x, error_y";
                        for (uint64_t k = 0; k < vTraj.size(); k++)
                        {
                            MatXd avg = gp.calcAvgTrajectory(vt, k).block(0,1, vt.size(),5);
                            jsonGP["Single"]["interpolation"].append(jsonEigen(avg));
                        }
                    }
                } 

                if (checkCoupled)
                {
                    double loc[6];
                    jsonGP["Corrected"]["dynamics_columns"] = "channel, particle_id, D, A";

                    GPT::GP_FBM::CDA* cda = gp.coupledModel();
                    for (uint64_t k = 0; k < vTraj.size(); k++)
                    {
                        loc[0] = double(vecID[k].trackID);
                        loc[1] = double(vecID[k].trajID);
                        loc[2] = CONV * cda->da[k].D;
                        loc[3] = cda->da[k].A;

                        jsonGP["Corrected"]["dynamics"].append(jsonArray(loc, 4));
                    }

                    jsonGP["Corrected"]["Substrate"]["DR"] = cda->DR;
                    jsonGP["Corrected"]["Substrate"]["AR"] = cda->AR;

                    if (checkSubstrate)
                    {
                        MatXd subs = gp.estimateSubstrateMovement();

                        jsonGP["Corrected"]["Substrate"]["trajectory_rows"] = "frame, time, pos_x, pos_y, error_x, error_y";
                        for (int64_t k = 0; k < subs.rows(); k++)
                        {
                            loc[0] = subs(k, 0);
                            loc[1] = subs(k, 1);
                            loc[2] = subs(k, 2);
                            loc[3] = subs(k, 3);
                            loc[4] = subs(k, 4);
                            loc[5] = subs(k, 5);

                            jsonGP["Corrected"]["Substrate"]["trajectory"].append(jsonArray(loc, 6));
                        }
                    }
                } // checkCoupled

            } // runGPFBM


        } // runTrajectories

        // Let's check on the alignment section
        if (runAlignment)
        {
            if (alignID == ALIGN::INDIVIDUAL)
            {
                std::vector<std::vector<MatXd>> vImg(numChannels, std::vector<MatXd>(2));

                for (int32_t ch = 0; ch < numChannels; ch++)
                {
                    vImg[ch][0] = mov.getImage(ch, 0);
                    vImg[ch][1] = mov.getImage(ch, meta.SizeT - 1);
                }


                for (int32_t ch = 1; ch < numChannels; ch++)
                {
                    GPT::Align var(2, vImg[0].data(), vImg[ch].data());

                    if (checkCamera)
                        var.alignCameras();

                    if (checkAberration)
                        var.correctAberrations();

                    const GPT::TransformData& RT = var.getTransformData();

                    Json::Value& jsonAlign = local["Alignment"]["channel_" + std::to_string(ch)];
                    jsonAlign["translate"] = jsonArray(RT.translate.data(), 2);
                    jsonAlign["rotate"] = jsonArray(RT.rotate.data(), 3);
                    jsonAlign["scale"] = jsonArray(RT.scale.data(), 2);
                    jsonAlign["size"] = jsonArray(RT.size.data(), 2);
                    jsonAlign["transform"] = jsonEigen(RT.trf.transpose());
                }

            }
            else if (alignID == ALIGN::BUNDLED)
                for (int32_t ch = 0; ch < numChannels; ch++)
                    vecImagesToAlign[ch][id] = mov.getImage(ch, 0);

        } // runAlignment

       
        if (prog)
            prog->progress = float(id) / float(vecSamples.size());
    }

    if (prog)
        prog->progress = 1.0f;

}
#include "ui.h"

#include <iostream>

#include "structure.hpp"
#include "childWindows.h"

#include <json/json.h>
#include "ghdf5.hpp"

// properties callback
static void propsDisplay_Movie(void *);
static void propsDisplay_Alignment(void *);
static void propsDisplay_Trajectories(void *);
static void propsDisplay_GProcess(void *);

static void viewPortEvents(UI &);
static void onUserUpdate(UI &);
static void ImGuiMenuBarLayer(UI &);
static void ImGuiLayer(UI &);

int main(void)
{
    MainStruct data;

    // Insert pluging
    data.activePlugin = "MOVIE";
    data.mPlugin["MOVIE"] = {true, propsDisplay_Movie};
    data.mPlugin["ALIGNMENT"] = {false, propsDisplay_Alignment};
    data.mPlugin["TRAJECTORIES"] = {true, propsDisplay_Trajectories};
    data.mPlugin["GPROCESS"] = {true, propsDisplay_GProcess};

    // Renderer
    UI ui("GP-Tool", 1200, 800);

    ui.mWindows["openTraj"] = std::make_unique<winOpenTrajectory>(&ui);
    ui.mWindows["trajectory"] = std::make_unique<winTrajectory>(&ui);
    ui.mWindows["substrate"] = std::make_unique<winSubstrate>(&ui);
    ui.mWindows["plotTraj"] = std::make_unique<winPlotTrajectory>();
    ui.mWindows["plotDistrib"] = std::make_unique<winPlotDistribution>(&data);

    ui.setUserData(&data);
    ui.FBuffer["viewport"] = std::make_unique<Framebuffer>(1200, 800); // viewport

    // Window

    // Setup main functions and run main loop
    ui.mainLoop(onUserUpdate, viewPortEvents, ImGuiMenuBarLayer, ImGuiLayer);

    return EXIT_SUCCESS;
} // main function

/*****************************************************************************/
/*****************************************************************************/
// HELPER FUNCTIONS

static void saveJSON(const String &path, UI *ui)
{
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui->getUserData());

    if (!data.align && data.vecGP.size() == 0 && !data.track)
    {
        ui->mWindows["inbox"]->show();
        ui->mail.createMessage<MSG_Warning>("Nothing to be saved!");
        return;
    }

    auto toArray = [](const double &a1, const double &a2) -> Json::Value {
        Json::Value vec;
        vec.append(a1);
        vec.append(a2);
        return vec;
    };

    auto convertEigenJSON = [](const MatrixXd &mat) -> Json::Value {
        Json::Value array;
        for (uint32_t k = 0; k < mat.rows(); k++)
        {
            Json::Value row;
            for (uint32_t l = 0; l < mat.cols(); l++)
                row.append(mat(k, l));

            array.append(row);
        }
        return array;
    };

    Json::Value output;
    const Metadata &meta = data.movie->getMetadata();
    output["movie_name"] = meta.movie_name;
    output["numChannels"] = meta.SizeC;

    String chNames = "";
    for (uint32_t k = 0; k < meta.SizeC; k++)
        chNames += meta.nameCH[k] + (k + 1 < meta.SizeC ? ", " : "");

    output["channels"] = "{" + chNames + "}";

    // First let's check if we have alignement matrix to store
    if (data.align)
        for (uint32_t ch = 1; ch < meta.SizeC; ch++)
        {
            const TransformData &RT = data.align->getTransformData(ch);

            Json::Value align;
            align["dimensions"] = toArray(meta.SizeX, meta.SizeY);
            align["translation"] = toArray(RT.dx, RT.dy);
            align["rotation"]["center"] = toArray(RT.cx, RT.cy);
            align["rotation"]["angle"] = RT.angle;
            align["scaling"] = toArray(RT.sx, RT.sy);
            align["transform"] = convertEigenJSON(RT.trf);

            output["Alignment"]["channel_" + std::to_string(ch)] = std::move(align);

        } // if-align

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////
    // Storing trajectories

    if (data.track)
    {
        Json::Value traj;
        traj["PhysicalSizeXY"] = meta.PhysicalSizeXY;
        traj["PhysicalSizeXYUnit"] = meta.PhysicalSizeXYUnit;
        traj["TimeIncrementUnit"] = meta.TimeIncrementUnit;
        traj["columns"] = "{frame, time, pos_x, error_x, pos_y, error_y,"
                          "size_x, size_y, signal, background}";

        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            const String ch_name = "channel_" + std::to_string(ch);
            std::vector<MatrixXd> &vTraj = data.track->getTrack(ch).traj;

            for (uint32_t k = 0; k < vTraj.size(); k++)
                traj[ch_name]["traj_" + std::to_string(k)] = convertEigenJSON(vTraj[k]);

        } // loop-channels

        output["Trajectories"] = std::move(traj);
    } // if-trajectories

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    // Storing cells

    const uint32_t nCells = data.vecGP.size();
    const double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

    for (uint32_t id = 0; id < nCells; id++)
    {
        GP_FBM &gp = data.vecGP[id];

        Json::Value cell;
        const uint32_t nParticles = gp.getNumParticles();

        // Now we save information about D and alpha
        if (gp.hasSingle())
        {
            Json::Value single;
            single["D_units"] = meta.PhysicalSizeXYUnit + "^2/" +
                                meta.TimeIncrementUnit + "^A";
            single["point_at_zero_units"] = "pixels";
            single["columns"] = "{channel, particle_id, D, A, zero_x, zero_y}";

            uint32_t k = 0;
            MatrixXd mat(nParticles, 6);
            const ParamDA *da = gp.getSingleParameters();
            for (auto &[ch, id] : gp.getParticleID())
            {
                mat.row(k) << ch, id, (Dcalib * da[k].D), da[k].A, da[k].mu[0], da[k].mu[1];
                k++;
            }
            single["dynamics"] = convertEigenJSON(mat);

            cell["without_substrate"] = std::move(single);
        } // single-model

        if (gp.hasCoupled())
        {
            Json::Value coupled;
            coupled["D_units"] = meta.PhysicalSizeXYUnit + "^2/" +
                                 meta.TimeIncrementUnit + "^A";
            coupled["columns"] = "{channel, particle_id, D, A}";

            uint32_t k = 0;
            MatrixXd mat(nParticles, 4);
            const ParamCDA &cda = gp.getCoupledParameters();
            for (auto &[ch, id] : gp.getParticleID())
            {
                mat.row(k) << ch, id, (Dcalib * cda.particle[k].D), cda.particle[k].A;
                k++;
            }

            coupled["dynamics"] = convertEigenJSON(mat);

            // Substrate
            coupled["substrate"]["dynamics"] = toArray(Dcalib * cda.DR, cda.AR);

            if (!gp.hasSubstrate())
                gp.estimateSubstrateMovement();

            coupled["substrate"]["trajectory"] = convertEigenJSON(gp.getSubstrate());

            cell["with_substrate"] = std::move(coupled);

        } // coupled-model

        output["Cells"].append(std::move(cell));

    } // loop-cells

    std::fstream arq(path, std::fstream::out);
    arq << output;
    arq.close();

} // saveJSON

static void saveHDF5(const String &path, UI *ui)
{
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui->getUserData());
    if (!data.align && data.vecGP.size() == 0 && !data.track)
    {
        ui->mWindows["inbox"]->show();
        ui->mail.createMessage<MSG_Warning>("Nothing to be saved!");
        return;
    }

    GHDF5 output;

    // Saving channel names
    const Metadata &meta = data.movie->getMetadata();
    output.setAttribute<String>("movie_name", meta.movie_name);
    output.setAttribute<uint32_t>("numChannels", meta.SizeC);

    String chNames = "";
    for (uint32_t k = 0; k < meta.SizeC; k++)
        chNames += meta.nameCH[k] + (k + 1 < meta.SizeC ? ", " : "");

    output.setAttribute<String>("channels", "{" + chNames + "}");

    // First let's check if we have alignement matrix to store
    if (data.align)
        for (uint32_t ch = 1; ch < meta.SizeC; ch++)
        {
            const TransformData &RT = data.align->getTransformData(ch);

            GHDF5 &align = output["Alignment"]["channel_" + std::to_string(ch)];

            uint32_t dim[] = {meta.SizeX, meta.SizeY};
            align["dimensions"].createFromPointer(dim, 2);

            std::array<double, 2> arr = {RT.dx, RT.dy};
            align["translation"].createFromPointer(arr.data(), 2);

            arr = {RT.cx, RT.cy};
            align["rotation"]["center"].createFromPointer(arr.data(), 2);
            align["rotation"]["angle"] = RT.angle;

            arr = {RT.sx, RT.sy};
            align["scaling"].createFromPointer(arr.data(), 2);

            MatrixXd trans = RT.trf.transpose();
            align["transform"].createFromPointer(trans.data(), trans.rows(), trans.cols());

        } // if-align

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    // Storing trajectories
    if (data.track)
    {
        GHDF5 &traj = output["Trajectories"];
        traj.setAttribute("PhysicalSizeXY", meta.PhysicalSizeXY);
        traj.setAttribute("PhysicalSizeXYUnit", meta.PhysicalSizeXYUnit);
        traj.setAttribute("TimeIncrementUnit", meta.TimeIncrementUnit);
        traj.setAttribute("columns", "{frame, time, pos_x, error_x, pos_y, error_y,"
                                     "size_x, size_y, signal, background}");

        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            const String ch_name = "channel_" + std::to_string(ch);
            std::vector<MatrixXd> &vTraj = data.track->getTrack(ch).traj;

            for (uint32_t k = 0; k < vTraj.size(); k++)
            {
                String txt = "traj_" + std::to_string(k);
                MatrixXd mat = vTraj.at(k).transpose();
                traj[ch_name][txt].createFromPointer(mat.data(), mat.rows(), mat.cols());
            } // loop-trajectories

        } // loop-channels

    } // if-trajectories

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    // Storing cells
    const uint32_t nCells = data.vecGP.size();
    const double Dcalib = meta.PhysicalSizeXY * meta.PhysicalSizeXY;

    for (uint32_t id = 0; id < nCells; id++)
    {
        GP_FBM &gp = data.vecGP[id];

        GHDF5 &cell = output["Cells"]["cell_" + std::to_string(id)];

        // First let's save trajectories
        const uint32_t nParticles = gp.getNumParticles();

        // Now we save information about D and alpha
        if (gp.hasSingle())
        {
            GHDF5 &single = cell["without_substrate"];

            uint32_t k = 0;
            MatrixXd mat(nParticles, 6);
            const ParamDA *da = gp.getSingleParameters();
            for (auto &[ch, id] : gp.getParticleID())
            {
                mat.row(k) << ch, id, (Dcalib * da[k].D), da[k].A, da[k].mu[0], da[k].mu[1];
                k++;
            }

            MatrixXd trans = mat.transpose();
            single["dynamics"].createFromPointer(trans.data(),
                                                 trans.rows(), trans.cols());

            single["dynamics"].setAttribute("point_at_zero_units", "pixels");
            single["dynamics"].setAttribute("columns",
                                            "{channel, particle_id, D, A, zero_x, zero_y}");

            single["dynamics"].setAttribute("D_units",
                                            meta.PhysicalSizeXYUnit + "^2/" +
                                                meta.TimeIncrementUnit + "^A");

        } // single-model

        if (gp.hasCoupled())
        {
            GHDF5 &coupled = cell["with_substrate"];

            uint32_t k = 0;
            MatrixXd mat(nParticles, 4);
            const ParamCDA &cda = gp.getCoupledParameters();
            for (auto &[ch, id] : gp.getParticleID())
            {
                mat.row(k) << ch, id, (Dcalib * cda.particle[k].D), cda.particle[k].A;
                k++;
            }

            MatrixXd trans = mat.transpose();
            coupled["dynamics"].createFromPointer(trans.data(),
                                                  trans.rows(), trans.cols());

            coupled["dynamics"].setAttribute("columns", "{channel, particle_id, D, A}");
            coupled["dynamics"].setAttribute("D_units", meta.PhysicalSizeXYUnit + "^2/" +
                                                            meta.TimeIncrementUnit + "^A");

            // Substrate
            double arr[] = {Dcalib * cda.DR, cda.AR};
            coupled["substrate"]["dynamics"].createFromPointer(arr, 2);

            coupled["substrate"]["dynamics"].setAttribute("columns", "{D, A}");
            coupled["substrate"]["dynamics"].setAttribute("D_units",
                                                          meta.PhysicalSizeXYUnit + "^2/" +
                                                              meta.TimeIncrementUnit + "^A");

            if (!gp.hasSubstrate())
                gp.estimateSubstrateMovement();

            trans = gp.getSubstrate().transpose();
            coupled["substrate"]["trajectory"].createFromPointer(trans.data(),
                                                                 trans.rows(),
                                                                 trans.cols());

        } // coupled-model

    } // loop-cells

    // Saving to file
    output.save(path);
} // saveHDF5

static void saveData(const String &path, UI *ui)
{
    // Generic function to save data based on extension
    ui->mWindows["inbox"]->show();

    if (path.find(".hdf") != String::npos)
        saveHDF5(path, ui);

    else if (path.find(".json") != String::npos)
        saveJSON(path, ui);

    else
    {
        ui->mail.createMessage<MSG_Warning>("Cannot save data to '" + path + "'");
        return;
    }

    ui->mail.createMessage<MSG_Info>("Data saved to " + path);

} // saveDataDialog

static void openMovie(const String &path, UI *ui)
{

    auto movieWorker = [](const String &address, UI *ui) -> void {
        MainStruct *data = reinterpret_cast<MainStruct *>(ui->getUserData());
        Mailbox *box = &(ui->mail);

        // Movie movie(box);
        data->movie = std::make_unique<Movie>(box);

        if (!data->movie->loadMovie(address))
            return;

        const Metadata &meta = data->movie->getMetadata();

        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            const MatrixXd &mat = data->movie->getImage(ch, data->frame);
            float low = mat.minCoeff(), high = mat.maxCoeff();

            data->iprops.setProps(ch);
            data->iprops.setConstrast(ch, low, high);
            data->iprops.setMinMaxValues(ch, 0.8 * low, 1.2 * high);
        }

        // Creating alignment plugin if necessary
        if (meta.SizeC > 1)
        {
            data->mPlugin["ALIGNMENT"].active = true;
            data->align = std::make_unique<Align>(box, meta.SizeC);
            data->align->setChannelToAlign(1);

            for (uint32_t ch = 1; ch < meta.SizeC; ch++)
                data->align->getTransformData(ch).reset(meta.SizeX, meta.SizeY);
        }

        data->createBuffers_flag = true;
        data->updateContrast_flag = true;
        data->updateViewport_flag = true;
    };

    // It might take awhile, so let's detach it and run on the side
    MainStruct *data = reinterpret_cast<MainStruct *>(ui->getUserData());

    // this will synchronize the parallel thread and main threadß
    data->openMovie_flag = true;

    for (auto &win : ui->mWindows)
        win.second->hide();

    data->resetData("MOVIE");
    data->mPlugin["ALIGNMENT"].active = false;

    ui->mWindows["inbox"]->show();
    std::thread(movieWorker, path, ui).detach();

} // openMovie

/*****************************************************************************/
/*****************************************************************************/
// PROPERTIES CALLBACKS

static void propsDisplay_Movie(void *ptr)
{
    UI &ui = *reinterpret_cast<UI *>(ptr);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());

    // can only be done in main thread
    if (data.createBuffers_flag)
    {
        data.openMovie_flag = false;
        data.createBuffers_flag = false;
        const Metadata &meta = data.movie->getMetadata();
        data.texID_signal = ui.tex.createDepthComponent(meta.SizeX, meta.SizeY);

        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            String txt = "contrast_" + std::to_string(ch);
            ui.FBuffer[txt] = std::make_unique<Framebuffer>(162, 100);
        }
    } // createBuffers

    if (!data.movie || data.openMovie_flag)
    {
        ImGui::Text("No movie is loaded!!");
        return;
    }

    const Metadata &meta = data.movie->getMetadata();
    size_t pos = meta.movie_name.find_last_of("/");
    String txt = meta.movie_name.substr(pos + 1);

    ui.fonts.text("Name: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%s", txt.c_str());

    ui.fonts.text("Acquisition date: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%s", meta.acquisitionDate.c_str());

    ui.fonts.text("Significant bits: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SignificantBits);

    ui.fonts.text("Width: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeX);

    ui.fonts.text("Height: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeY);

    ui.fonts.text("Frames: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", meta.SizeT);

    ui.fonts.text("Calibration: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%.3f %s", meta.PhysicalSizeXY, meta.PhysicalSizeXYUnit.c_str());

    ImGui::Spacing();
    ui.fonts.text("Channels: ", "bold");
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        txt = "   :: " + meta.nameCH[ch];
        ImGui::Text("%s", txt.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    int frame = data.frame;
    if (ImGui::SliderInt("Frame", &frame, 0, meta.SizeT - 1))
    {
        data.frame = frame;
        data.updateContrast_flag = true;
        data.updateViewport_flag = true;
    }

    bool check = true;
    check &= data.imgPos.x > 0 && data.imgPos.x < 1.0f;
    check &= data.imgPos.y > 0 && data.imgPos.y < 1.0f;
    if (check && data.viewHover)
    {
        float FX = meta.SizeX * data.imgPos.x,
              FY = meta.SizeY * data.imgPos.y;

        ImGui::Text("Position: %.2f x %.2f %s :: %d x %d",
                    meta.PhysicalSizeXY * FX, meta.PhysicalSizeXY * FY,
                    meta.PhysicalSizeXYUnit.c_str(),
                    int(FX), int(FY));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Color map:");

    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        txt = "Ch " + std::to_string(ch);
        String current = data.iprops.getName(ch);

        ImGui::PushID(txt.c_str());

        if (ImGui::BeginCombo(txt.c_str(), current.c_str()))
        {
            for (const String &name : data.iprops.getList())
            {
                bool selected = name.compare(current) == 0;
                if (ImGui::Selectable(name.c_str(), &selected))
                    data.iprops.setLUT(ch, name);

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
            data.updateViewport_flag = true;
            data.updateContrast_flag = true;

            ImGui::EndCombo();
        }

        if (current.compare("None") != 0)
        {
            String name = "contrast_" + std::to_string(ch);

            float port = ImGui::GetContentRegionAvail().x,
                  wid = ui.FBuffer[name]->getWidth(),
                  hei = ui.FBuffer[name]->getHeight();

            ImGui::Image((void *)(uintptr_t)ui.FBuffer[name]->getID(), {wid, hei});

            if (port != wid)
            {
                wid = port;
                ui.FBuffer[name] = std::make_unique<Framebuffer>(wid, hei);
                data.updateContrast_flag = true;
            }

            // Update contrast
            if (ui.mouse.leftPressed && ImGui::IsItemHovered())
            {
                const ImVec2 &minPos = ImGui::GetItemRectMin(),
                             &maxPos = ImGui::GetItemRectMax(),
                             &pos = ImGui::GetMousePos();

                auto [low, high] = data.iprops.getContrast(ch);
                auto &[minValue, maxValue] = data.iprops.getMinMaxValues(ch);

                float dx = (pos.x - minPos.x) / (maxPos.x - minPos.x);
                dx = (maxValue - minValue) * dx + minValue;

                if (abs(dx - low) < abs(dx - high))
                    low = dx;
                else
                    high = dx;

                data.iprops.setConstrast(ch, low, high);

                data.updateContrast_flag = true;
                data.updateViewport_flag = true;
            } // if-contrastWidget Changes
        }

        ImGui::PopID();

        ImGui::Spacing();
        ImGui::Spacing();

    } // loop-channels

} // propsDisplay_Movie

static void propsDisplay_Alignment(void *ptr)
{
    UI &ui = *reinterpret_cast<UI *>(ptr);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());

    auto runAlignment = [](Mailbox *box, MainStruct *data) -> void {
        uint32_t chAlign = data->align->getChannelIndex();

        data->align->setImageData(data->movie->getMetadata().SizeT,
                                  data->movie->getChannel(0),
                                  data->movie->getChannel(chAlign));

        MSG_Timer *msg2 = box->createMessage<MSG_Timer>("Aligning cameras");
        bool check = data->align->alignCameras();
        msg2->markAsRead();

        if (check)
        {
            Message *msg3 = box->createMessage<MSG_Timer>("Correcting chromatic aberrations");
            data->align->correctAberrations();
            msg3->markAsRead();
        }

        // worker is finished
        data->updateViewport_flag = true;
    };

    auto item = [](uint32_t &val, uint32_t nChannels, const char *title) -> bool {
        String txt = "Channel " + std::to_string(val);

        bool out = false;
        //TODO: Change color to red if values are equal

        if (ImGui::BeginCombo(title, txt.c_str()))
        {
            for (uint32_t ch = 1; ch < nChannels; ch++)
            {
                txt = "Channel " + std::to_string(ch);
                if (ImGui::Selectable(txt.c_str()))
                {
                    val = ch;
                    ImGui::SetItemDefaultFocus();
                    out = true;
                }
            }

            ImGui::EndCombo();
        }

        return out;
    };

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
        check |= ImGui::DragFloat(("##" + XYA).c_str(), &val, step, 0.0, 0.0, "%.4f");
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        return check;
    };

    /////////////////////////////////////////////////////////////////

    const uint32_t nChannels = data.movie->getMetadata().SizeC;
    uint32_t chAlign = data.align->getChannelIndex();

    if (item(chAlign, nChannels, "To align"))
    {
        data.align->setChannelToAlign(chAlign);
        data.updateViewport_flag = true;
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    TransformData &RT = data.align->getTransformData(chAlign);

    glm::vec2 trans = {RT.dx, RT.dy},
              rot = {RT.cx, RT.cy},
              scl = {RT.sx, RT.sy};

    float angle = RT.angle;

    bool check = false;
    ImGui::Columns(2);

    String name = "Translate";
    ImGui::Text("%s", name.c_str());
    ImGui::NextColumn();
    ImGui::PushID(name.c_str());
    float linewidth = ImGui::GetContentRegionAvailWidth();
    check |= fancyDrag("X", trans.x, 0.0f, 0.5f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", trans.y, 0.0f, 0.5f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    name = "Scale";
    ImGui::Text("%s", name.c_str());
    ImGui::NextColumn();
    ImGui::PushID(name.c_str());
    check |= fancyDrag("X", scl.x, 1.0f, 0.005f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", scl.y, 1.0f, 0.005f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    name = "Rotate";
    ImGui::Text("%s", name.c_str());
    ImGui::NextColumn();
    ImGui::PushID(name.c_str());
    check |= fancyDrag("X", rot.x, 0.5 * RT.width, 0.5f, linewidth);
    ImGui::SameLine();
    check |= fancyDrag("Y", rot.y, 0.5 * RT.height, 0.5f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    name = "Angle";
    ImGui::Text("%s", name.c_str());
    ImGui::NextColumn();
    ImGui::PushID(name.c_str());
    check |= fancyDrag("A", angle, 0.0f, 0.001f, linewidth);
    ImGui::PopID();
    ImGui::NextColumn();

    ImGui::Columns(1);

    if (check)
    {
        RT.dx = trans.x;
        RT.dy = trans.y;
        RT.cx = rot.x;
        RT.cy = rot.y;
        RT.angle = angle;
        RT.sx = scl.x;
        RT.sy = scl.y;
        RT.updateTransform();
        data.updateViewport_flag = true;
    }

    ImGui::Spacing();
    ImGui::Spacing();

    if (ImGui::Button("Auto alignment"))
    {
        ui.mWindows["inbox"]->show();
        std::thread(runAlignment, &(ui.mail), &data).detach();
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset"))
    {
        Metadata &meta = data.movie->getMetadata();
        RT.reset(meta.SizeX, meta.SizeY);
        data.updateViewport_flag = true;
    }

} // propsDisplay_Alignment

static void propsDisplay_Trajectories(void *ptr)
{
    UI &ui = *reinterpret_cast<UI *>(ptr);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());

    if (!data.track)
    {
        ImGui::Text("No trajectories loaded!!");
        return;
    }

    auto runEnhance = [](int rad, Mailbox *box, MainStruct *data) -> void {
        const uint32_t N = data->track->getNumTracks();
        std::vector<Track> help;
        for (uint32_t k = 0; k < N; k++)
            help.push_back(data->track->getTrack(k));

        Trajectory another(data->movie.get(), box);
        for (uint32_t k = 0; k < N; k++)
        {
            const Track &tr = data->track->getTrack(k);

            if (tr.path.find("xml") == String::npos)
                another.useCSV(tr.path, tr.channel);
            else
                another.useICY(tr.path, tr.channel);
        }

        another.setSpotRadius(rad);
        another.enhanceTracks();

        // To avoid seg faults, we should block updates
        data->mtx.lock();
        data->track = std::make_unique<Trajectory>(std::move(another));
        data->mtx.unlock();

        data->updateViewport_flag = true;
    };

    int rad = data.track->getSpotRadius();

    ImGui::Text("Spot radius");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::DragInt("##Spot radius", &rad, 0.2, 1, 10))
        data.track->setSpotRadius(rad);

    ImGui::SameLine();
    if (ImGui::Button("Run"))
    {
        ui.mWindows["inbox"]->show();
        std::thread(runEnhance, rad, &ui.mail, &data).detach();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    ///////////////////////////////////////////////////////
    // ROI

    if (!data.roi)
        ImGui::SetNextTreeNodeOpen(false);

    if (ImGui::TreeNode("Select ROI"))
    {
        data.updateViewport_flag = true;

        if (!data.roi)
            data.roi = std::make_unique<Roi>();

        ImGui::ColorEdit4("ROI color", &data.roi->cor.x);

        ImGui::Text("- Left click to introduce point\n"
                    "- Middle click to move points\n"
                    "- Right click to remove point");

        ImGui::Spacing();
        if (ImGui::Button("Select"))
        {
            const uint32_t nTracks = data.track->getNumTracks(),
                           nPoints = data.roi->points.size() / 2;
            const float wid = data.movie->getMetadata().SizeX,
                        hei = data.movie->getMetadata().SizeY;

            auto crossLine = [](glm::vec2 &p1, glm::vec2 &p2, glm::vec2 &pos) -> int {
                glm::vec2 dp = p2 - p1;
                glm::mat2 mat(pos.x, pos.y, -dp.x, -dp.y);

                if (glm::determinant(mat) == 0.0)
                    return 0;

                glm::vec2 su = glm::inverse(mat) * p1;
                if (su.x >= 0 && su.x <= 1.0 && su.y >= 0.0 && su.y <= 1.0)
                    return 1;
                else
                    return 0;
            };

            for (uint32_t tr = 0; tr < nTracks; tr++)
            {
                Track &track = data.track->getTrack(tr);
                const uint32_t nTraj = track.traj.size();
                for (uint32_t nt = 0; nt < nTraj; nt++)
                {
                    glm::vec2 pos = {track.traj[nt](0, TrajColumns::POSX) / wid,
                                     track.traj[nt](0, TrajColumns::POSY) / hei};

                    uint32_t inner = 0;
                    for (uint32_t pt = 0; pt < nPoints; pt++)
                    {
                        uint32_t k = (pt + 1) % nPoints;
                        glm::vec2 p1 = {data.roi->points[2 * pt],
                                        data.roi->points[2 * pt + 1]};

                        glm::vec2 p2 = {data.roi->points[2 * k],
                                        data.roi->points[2 * k + 1]};

                        inner += crossLine(p1, p2, pos);
                    }
                    track.view[nt].first = !(inner % 2 == 0);

                } // loop-trajectories

            } // loop-tracks

            data.roi = nullptr;
        } // if-button done

        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            data.roi = nullptr;

        ImGui::TreePop();

    } // Tree

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    ///////////////////////////////////////////////////////

    const uint32_t nChannels = data.movie->getMetadata().SizeC;

    ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
    if (ImGui::BeginTabBar("TrajTabBar", tab_bar_flags))
    {
        for (uint32_t ch = 0; ch < nChannels; ch++)
        {
            String txt = "Channel " + std::to_string(ch);
            if (ImGui::BeginTabItem(txt.c_str()))
            {
                ImGui::PushID(txt.c_str());

                String filename = data.track->getTrack(ch).path;
                size_t pos = filename.find_last_of("/");
                filename = filename.substr(pos + 1);

                ui.fonts.text("Filename: ", "bold");
                ImGui::SameLine();
                ImGui::Text("%s", filename.c_str());

                ui.fonts.text("Description: ", "bold");
                ImGui::SameLine();
                ImGui::Text("%s", data.track->getTrack(ch).description.c_str());

                ImGui::Spacing();
                ImGui::Spacing();

                std::vector<View> &view = data.track->getTrack(ch).view;
                ImGui::Text("Select: ");
                ImGui::SameLine();
                if (ImGui::Button("All"))
                {
                    for (uint32_t k = 0; k < view.size(); k++)
                        view[k].first = true;

                    data.updateViewport_flag = true;
                }

                ImGui::SameLine();
                if (ImGui::Button("None"))
                {
                    for (uint32_t k = 0; k < view.size(); k++)
                        view[k].first = false;

                    data.updateViewport_flag = true;
                }

                ImGui::Spacing();

                ImGui::Columns(2);
                for (uint32_t k = 0; k < view.size(); k++)
                {
                    auto &[show, cor] = view.at(k);
                    txt = "Trajectory " + std::to_string(k);
                    ImGui::PushID(txt.c_str());

                    if (ImGui::Checkbox("##check", &show))
                        data.updateViewport_flag = true;

                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, {cor[0], cor[1], cor[2], 1.0});
                    ui.fonts.text(txt, "bold");
                    ImGui::PopStyleColor();
                    ImGui::NextColumn();

                    if (ImGui::Button("Detail"))
                    {
                        winTrajectory *win = (winTrajectory *)ui.mWindows["trajectory"].get();
                        win->setInfo(ch, k);
                        win->show();
                    }

                    ImGui::NextColumn();

                    ImGui::PopID();
                }

                ImGui::Columns(1);

                ImGui::PopID();
                ImGui::EndTabItem();
            } // if-tab

        } // loop-channels

        ImGui::EndTabBar();
    } // If

} // propsDisplay_Trajectories

static void propsDisplay_GProcess(void *ptr)
{
    UI &ui = *reinterpret_cast<UI *>(ptr);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());

    if (!data.track)
    {
        ImGui::Text("No trajectories loaded!!");
        return;
    }

    ImGui::Text("- Infer parameters\n"
                "- Estimate distributitions\n"
                "- Expected trajectories\n"
                "- Substrate movement");

    ImGui::Spacing();

    auto createCell = [](std::vector<MatrixXd> vTraj,
                         const std::vector<std::pair<uint32_t, uint32_t>> &partID,
                         Mailbox *box, MainStruct *data) -> void {
        // We already calculated the basic stuff here

        Message *msg = box->createMessage<MSG_Timer>("Optimizing cell's parameters");

        const uint32_t N = vTraj.size();
        GP_FBM gp(vTraj, box);
        gp.setParticleID(partID);

        if (!gp.singleModel())
        {
            msg->markAsRead();
            return;
        }

        // Calculating average trajectories
        for (uint32_t k = 0; k < N; k++)
        {
            const VectorXd &vFrame = vTraj[k].col(TrajColumns::FRAME),
                           &vTime = vTraj[k].col(TrajColumns::TIME);

            const uint32_t tot = 5 * (vFrame(vFrame.size() - 1) - vFrame(0));

            VectorXd
                newFrame = VectorXd::LinSpaced(tot, vFrame(0), vFrame(vFrame.size() - 1)),
                newTime = VectorXd::LinSpaced(tot, vTime(0), vTime(vTime.size() - 1));

            gp.calcAvgTrajectory(newFrame, newTime, k);
        }

        if (N > 1)
            if (!gp.coupledModel())
            {
                msg->markAsRead();
                return;
            }

        data->mtx.lock();
        data->vecGP.emplace_back(gp);
        data->mtx.unlock();

        msg->markAsRead();
    };

    auto runSingleDistrib = [](GP_FBM *gp, Mailbox *box) -> void {
        MSG_Timer *msg =
            box->createMessage<MSG_Timer>("Calculating distribution for single particle");

        gp->distrib_singleModel();

        msg->markAsRead();
    };

    auto runCoupledDistrib = [](GP_FBM *gp, Mailbox *box) -> void {
        MSG_Timer *msg =
            box->createMessage<MSG_Timer>("Calculating distribution for multiple particles");

        gp->distrib_coupledModel();

        msg->markAsRead();
    };

    auto addRow = [](uint32_t id, double D, double A, const GP_FBM *gp,
                     winPlotTrajectory *win) -> void {
        ImGui::Text("%d", id);
        ImGui::NextColumn();

        ImGui::Text("%.3e", D);
        ImGui::NextColumn();

        ImGui::Text("%.3f", A);
        ImGui::NextColumn();

        char txt[32];
        sprintf(txt, "Particle %d", id);
        ImGui::PushID(txt);

        float width = ImGui::GetContentRegionAvailWidth();
        if (ImGui::Button("View", {width, 0}))
        {
            win->setInfo(&gp->getAvgTrajectory(id), "Movement");
            win->show();
        }

        ImGui::PopID();
        ImGui::NextColumn();

        ImGui::Spacing();
    };

    if (ImGui::Button("Add cell"))
    {
        ui.mWindows["inbox"]->show();

        // Checking which trajectories are selected
        std::vector<MatrixXd> vTraj;
        std::vector<std::pair<uint32_t, uint32_t>> partID;

        const uint32_t nTracks = data.track->getNumTracks();
        for (uint32_t ch = 0; ch < nTracks; ch++)
        {
            const Track &track = data.track->getTrack(ch);
            const uint32_t N = track.traj.size();

            // TODO: Correct trajectory for alignment when necessary
            for (uint32_t l = 0; l < N; l++)
                if (track.view[l].first)
                {
                    if (track.traj[l].rows() < 50)
                    {
                        ui.mail.createMessage<MSG_Warning>("Discarding trajectory with "
                                                           "less than 50 frames!");
                    }
                    else
                    {
                        vTraj.push_back(track.traj[l].block(0, 0, track.traj[l].rows(), 6));
                        partID.push_back({ch, l});
                    }
                }
        } // loop-tracks

        if (vTraj.size() == 0)
            ui.mail.createMessage<MSG_Warning>("No trajectories selected!!");

        else if (vTraj.size() > 5)
            ui.mail.createMessage<MSG_Warning>(
                "A maximum of 5 trajectories are allowed per cell!");

        else
            std::thread(createCell, vTraj, partID, &ui.mail, &data).detach();

    } // if-add-cell

    ImGui::Spacing();
    ImGui::Spacing();

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    const float
        pix2mu = data.movie->getMetadata().PhysicalSizeXY,
        DCalib = pix2mu * pix2mu;

    const char
        *spaceUnit = data.movie->getMetadata().PhysicalSizeXYUnit.c_str(),
        *timeUnit = data.movie->getMetadata().TimeIncrementUnit.c_str();

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_None;
    nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    nodeFlags |= ImGuiTreeNodeFlags_Framed;
    nodeFlags |= ImGuiTreeNodeFlags_FramePadding;
    nodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    nodeFlags |= ImGuiTreeNodeFlags_AllowItemOverlap;

    for (uint32_t id = 0; id < data.vecGP.size(); id++)
    {
        String txt = "Cell " + std::to_string(id);
        bool openTree = ImGui::TreeNodeEx(txt.c_str(), nodeFlags);

        float fSize = ImGui::GetContentRegionAvailWidth() - 6 * ImGui::GetFontSize();
        ImGui::SameLine(fSize);

        if (ImGui::Button("Select"))
        {
            // Setting everything to false
            for (uint32_t ch = 0; ch < data.track->getNumTracks(); ch++)
                for (auto &view : data.track->getTrack(ch).view)
                    view.first = false;

            // Activating only important ones
            for (auto [ch, pt] : data.vecGP[id].getParticleID())
                data.track->getTrack(ch).view[pt].first = true;

            data.updateViewport_flag = true;
        }

        ImGui::SameLine();
        if (ImGui::Button("Remove"))
        {
            ui.mWindows["substrate"]->hide();
            ui.mWindows["plotDistrib"]->hide();

            openTree = false;
            data.vecGP.erase(data.vecGP.begin() + id);
        }

        if (openTree)
        {
            const GP_FBM &gp = data.vecGP[id];
            const uint32_t nParticles = gp.getNumParticles();

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

            winPlotTrajectory *winPlot = (winPlotTrajectory *)ui.mWindows["plotTraj"].get();
            if (nParticles == 1 && gp.hasSingle())
            {
                const ParamDA &da = gp.getSingleParameters()[0];
                addRow(0, DCalib * da.D, da.A, &gp, winPlot);
            } // if-single-particle

            if (nParticles > 1 && gp.hasCoupled())
            {
                const ParamCDA &cda = gp.getCoupledParameters();
                for (uint32_t pt = 0; pt < nParticles; pt++)
                    addRow(pt, DCalib * cda.particle[pt].D, cda.particle[pt].A, &gp, winPlot);

            } // if-multiple particles

            ImGui::Columns(1);
            ImGui::Separator();

            // float width = ImGui::GetContentRegionAvailWidth();
            if (ImGui::Button("View distribution"))
            {
                winPlotDistribution *win = (winPlotDistribution *)ui.mWindows["plotDistrib"].get();
                win->setCellID(id);
                win->show();

                GP_FBM *gp = &data.vecGP[id];
                Mailbox *box = &ui.mail;

                if (nParticles == 1 && !data.vecGP[id].hasDistribSingle())
                {
                    std::thread(runSingleDistrib, gp, box).detach();
                    ui.mWindows["inbox"]->show();
                }

                if (nParticles > 1 && !data.vecGP[id].hasDistribCoupled())
                {
                    std::thread(runCoupledDistrib, gp, box).detach();
                    ui.mWindows["inbox"]->show();
                }

            } // button-distribution

            if (nParticles > 1)
            {
                ImGui::SameLine();
                if (ImGui::Button("Substrate"))
                {
                    winSubstrate *win = (winSubstrate *)ui.mWindows["substrate"].get();
                    win->setCellID(id);
                    win->show();

                    if (!gp.hasSubstrate())
                        data.vecGP[id].estimateSubstrateMovement();

                } // button-substrate
            }

            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TreePop();
        } // if-treenode

    } // loop-cells

} // propsDisplay_GProcess

/*****************************************************************************/
/*****************************************************************************/
// UI

static void viewPortEvents(UI &ui)
{
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());

    if (ui.resizeEvent)
        data.updateViewport_flag = true;

    if (data.viewHover)
    {
        // move roi points
        if (data.roi && ui.mouse.middlePressed)
            for (uint32_t k = 0; k < data.roi->points.size(); k += 2)
            {
                glm::vec2 pos(data.roi->points[k], data.roi->points[k + 1]);
                if (glm::distance(data.imgPos, pos) < 0.01)
                {
                    data.roi->points[k] = data.imgPos.x;
                    data.roi->points[k + 1] = data.imgPos.y;
                    break;
                }
            }

        // move camera -- add roi points
        if (ui.mouse.leftPressed && !data.roi)
        {
            data.updateViewport_flag = true;
            glm::vec2 dr = ui.elapsedTime * ui.mouse.dpos;
            ui.cam.moveHorizontal(dr.x);
            ui.cam.moveVertical(dr.y);
        } // left-button-pressed

        if (data.roi && ui.mouse.leftButton &&
            ui.mouse.action == GLFW_RELEASE && data.roi->points.size() < 40)
        {
            data.updateViewport_flag = true;
            data.roi->points.push_back(data.imgPos.x);
            data.roi->points.push_back(data.imgPos.y);
        } // roi-actions

        // remove roi points
        if (ui.mouse.rightPressed)
        {
            if (data.roi)
                for (uint32_t k = 0; k < data.roi->points.size(); k += 2)
                {
                    glm::vec2 pos(data.roi->points[k], data.roi->points[k + 1]);
                    if (glm::distance(data.imgPos, pos) < 0.01)
                    {
                        data.roi->points.erase(data.roi->points.begin() + k + 1);
                        data.roi->points.erase(data.roi->points.begin() + k);
                        break;
                    }
                }

        } // right-button

        // zoom
        if (ui.mouse.offset.y > 0.0)
        {
            ui.cam.moveFront(ui.elapsedTime);
            data.updateViewport_flag = true;
        }
        else if (ui.mouse.offset.y < 0.0)
        {
            ui.cam.moveBack(ui.elapsedTime);

            data.updateViewport_flag = true;
        }

    } // viewport

} // control

static void onUserUpdate(UI &ui)
{
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());

    // quick and dirty background color if nothing is loaded
    if (!data.movie || data.openMovie_flag)
    {
        ui.FBuffer["viewport"]->bind();
        glad_glClear(GL_COLOR_BUFFER_BIT);
        glad_glClearColor(0.6, 0.6, 0.6, 1.0);
        ui.FBuffer["viewport"]->unbind();
        return;
    }

    const Metadata &meta = data.movie->getMetadata();

    // Camera transform
    float aRatio = float(data.viewport.x) / float(data.viewport.y);
    glm::mat4 trf = ui.cam.getViewMatrix(aRatio);

    // Scaling quad to image aspect ratio
    aRatio = meta.SizeY / float(meta.SizeX);
    trf *= glm::scale(glm::mat4(1), glm::vec3{1.0, aRatio, 1.0f});

    // Updating over picture pointer
    float z = ui.cam.getZoom();
    glm::vec2 botRect = glm::vec2(trf * glm::vec4{-0.5, -0.5, 0.0, 1.0} / z),
              topRect = glm::vec2(trf * glm::vec4{+0.5, +0.5, 0.0, 1.0} / z);

    data.imgPos = {(data.viewPos.x - botRect.x) / (topRect.x - botRect.x),
                   (data.viewPos.y - botRect.y) / (topRect.y - botRect.y)};

    // some optimization to use less cpu/gpu
    if (!data.updateViewport_flag && !data.updateContrast_flag)
        return;

    /////////////////////////////////////////////////////
    // VIEWPORT->Runs with mouse hover, frame change, contrast change, alignment, winResize

    glm::vec2 dim = {meta.SizeX, meta.SizeY};

    ui.FBuffer["viewport"]->bind();

    glad_glClear(GL_COLOR_BUFFER_BIT);
    glad_glClearColor(0.6, 0.6, 0.6, 1.0);

    ///////////////////////////////////////////////////////

    // Setting up shader
    ui.shader.useProgram("viewport");
    ui.shader.setMatrix4f("u_transform", glm::value_ptr(trf));
    ui.shader.setInteger("u_signal", 0);
    ui.tex.bind(data.texID_signal, 0);
    ui.shader.setVec2f("dim", &dim[0]);

    // Let's clear a rectangle for proper blending
    ui.shader.setInteger("clear", true);
    ui.quad.draw();
    ui.shader.setInteger("clear", false);

    // Enable blending
    glad_glEnable(GL_BLEND);
    glad_glBlendFunc(GL_ONE, GL_ONE);

    // Blending channels
    for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    {
        // MatrixXf is internally set to be row major
        MatrixXf mat = data.movie->getImage(ch, data.frame).cast<float>();

        // Working on contrast and updating texture
        auto &[low, high] = data.iprops.getContrast(ch);
        mat = (mat.array() - low) / (high - low);

        // Using depth component as single channel buffer for signal
        ui.tex.updateDepthComponent(data.texID_signal, mat.data());

        // Determining if channel should be aligned
        MatrixXf itrf = MatrixXf::Identity(3, 3);
        if (data.align && ch > 0)
            itrf = data.align->getTransformData(ch).itrf.cast<float>();

        ui.shader.setMatrix3f("u_align", itrf.data());

        // Updating colormap and blending channel
        ui.shader.setVec3f("colormap", data.iprops.getLUT(ch).data());
        ui.quad.draw();

    } // loop-channels

    ///////////////////////////////////////////////////////
    // DRAWING A ELLIPSE AROUND SELECTED TRAJECTORIES
    if (data.track)
    {
        ui.shader.useProgram("trajectory");
        ui.shader.setMatrix4f("u_transform", glm::value_ptr(trf));

        glad_glEnable(GL_BLEND);
        glad_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            const uint32_t N = data.track->getTrack(ch).traj.size();
            for (uint32_t k = 0; k < N; k++)
            {
                auto &[show, cor] = data.track->getTrack(ch).view[k];

                if (show)
                {
                    const MatrixXd &mat = data.track->getTrack(ch).traj[k];

                    MatrixXd trans = MatrixXd::Identity(3, 3);
                    if (data.align && ch > 0)
                        trans = data.align->getTransformData(ch).trf;

                    for (uint32_t l = 0; l < mat.rows(); l++)
                        if (uint32_t(mat(l, TrajColumns::FRAME)) == data.frame)
                        {

                            glm::vec2 pos;
                            pos.x = trans(0, 0) * mat(l, TrajColumns::POSX) +
                                    trans(0, 1) * mat(l, TrajColumns::POSY) +
                                    trans(0, 2);

                            pos.y = trans(1, 0) * mat(l, TrajColumns::POSX) +
                                    trans(1, 1) * mat(l, TrajColumns::POSY) +
                                    trans(1, 2);

                            pos /= dim; // Normalize for texture space

                            float rad = 0.5 * glm::length(glm::vec2(
                                                  mat(l, TrajColumns::SIZEX) / dim.x,
                                                  mat(l, TrajColumns::SIZEY) / dim.y));

                            if (rad < 0.01f)
                                rad = 0.5f * data.track->getSpotRadius() / dim.x;

                            ui.shader.setVec3f("color", cor.data());
                            ui.shader.setVec2f("position", glm::value_ptr(pos));
                            ui.shader.setFloat("radius", rad);
                            ui.quad.draw();

                            break;
                        } // if-frame exists

                } // if-show-trajectory

            } // loop-trajectories

        } // loop-channels

    } // if-has trajectories

    glad_glDisable(GL_BLEND);

    ///////////////////////////////////////////////////////
    // DRAWING ROI IF NECESSARY
    if (data.roi)
    {
        uint32_t nPoints = 0.5f * data.roi->points.size();
        if (nPoints > 0)
        {
            glad_glEnable(GL_BLEND);
            glad_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            ui.shader.useProgram("selectRoi");
            ui.shader.setMatrix4f("u_transform", glm::value_ptr(trf));
            ui.shader.setVec4f("color", &data.roi->cor.x);

            ui.shader.setInteger("nPoints", nPoints);
            ui.shader.setVec2fArray("u_points", data.roi->points.data(), nPoints);

            ui.quad.draw();

            glad_glDisable(GL_BLEND);
        }

    } // if-roi

    // Done with viewport
    ui.FBuffer["viewport"]->unbind();

    //////////////////////////////////////////////////////
    // CONSTRAST HISTOGRAMS

    if (data.updateContrast_flag)
    {
        const Metadata &meta = data.movie->getMetadata();
        const size_t N = meta.SizeX * meta.SizeY;
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            Histogram &histo = data.iprops.getHistogram(ch);
            auto &[minValue, maxValue] = data.iprops.getMinMaxValues(ch);

            const double *mat = data.movie->getImage(ch, data.frame).data();

            histo.fill(0.0f);
            for (size_t k = 0; k < N; k++)
            {
                uint32_t val = 255.0f * (mat[k] - minValue) / (maxValue - minValue + 0.01f);
                histo[val]++;
            }

            float norma = std::accumulate(histo.begin(), histo.end(), 0.0f);
            for (uint32_t k = 0; k < 256; k++)
                histo[k] /= norma;

            // update histogram
            auto &[low, high] = data.iprops.getContrast(ch);
            const LUT &lut = data.iprops.getLUT(ch);
            glm::mat4 trf = glm::ortho(-0.5, 0.5, 0.5, -0.5);

            float lowContrast = (low - minValue) / (maxValue - minValue),
                  highContrast = (high - minValue) / (maxValue - minValue);

            ui.shader.useProgram("histogram");
            ui.shader.setMatrix4f("u_transform", glm::value_ptr(trf));
            ui.shader.setVec3f("color", lut.data());
            ui.shader.setFloatArray("bHeight", histo.data(), 256);
            ui.shader.setFloat("lowContrast", lowContrast);
            ui.shader.setFloat("highContrast", highContrast);

            String name = "contrast_" + std::to_string(ch);
            ui.FBuffer[name]->bind();
            ui.quad.draw();
            ui.FBuffer[name]->unbind();

        } // loop-channels

        data.updateContrast_flag = false;
    } // if-updateContrast

    data.updateContrast_flag = false;
    data.updateViewport_flag = false;
} // onUserUpdate

static void ImGuiLayer(UI &ui)
{
    MainStruct *data = reinterpret_cast<MainStruct *>(ui.getUserData());

    // External windows
    for (auto &win : ui.mWindows)
        if (win.second->active)
            win.second->display();

    // ///////////////////////////////////////////////////////

    ImGui::Begin("Plugins");

    float width = ImGui::GetContentRegionAvailWidth();
    ImVec2 buttonSize{width, 30};
    ImVec4
        chosenColor{0.1, 0.6, 0.1, 1.0},
        hoverColor{0.2, 0.7, 0.2, 1.0};

    auto coloredButton = [&](const String &label) {
        bool check = data->activePlugin.compare(label) == 0;

        if (check)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, chosenColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
        }

        if (ImGui::Button(label.c_str(), buttonSize))
            data->activePlugin = label;

        if (check)
            ImGui::PopStyleColor(3);
    };

    ui.fonts.push("bold");

    for (auto &[name, plugin] : data->mPlugin)
        if (plugin.active)
            coloredButton(name);

    ui.fonts.pop();

    ImGui::End();

    // ///////////////////////////////////////////////////////

    ImGui::Begin("Properties");
    data->mPlugin[data->activePlugin].callback(&ui);
    ImGui::End();

    ///////////////////////////////////////////////////////

    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoTitleBar);

    ImVec2 port = ImGui::GetContentRegionAvail();
    ImGui::Image((void *)(uintptr_t)ui.FBuffer["viewport"]->getID(), port);

    ImVec2 pos = ImGui::GetMousePos(), rect = ImGui::GetItemRectMin(),
           dim = ImGui::GetItemRectSize();

    data->viewPos = {2.0f * (pos.x - rect.x) / dim.x - 1.0f,
                     2.0f * (pos.y - rect.y) / dim.y - 1.0f};

    if (int(port.x) != data->viewport.x || int(port.y) != data->viewport.y)
    {
        data->viewport.x = int(port.x);
        data->viewport.y = int(port.y);
        ui.FBuffer["viewport"] = std::make_unique<Framebuffer>(data->viewport.x, data->viewport.y);

        data->updateViewport_flag = true;
    }

    data->viewHover = ImGui::IsWindowHovered();
    ImGui::End();

} // UILayer

static void ImGuiMenuBarLayer(UI &ui)
{

    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open movie..."))
        {
            FileDialog *dialog = reinterpret_cast<FileDialog *>(ui.mWindows["dialog"].get());
            dialog->createDialog(DIALOG_OPEN, "Choose TIF file...", {".tif", ".ome.tif"});

            dialog->callback(openMovie, &ui);
        }

        bool enable = data.movie ? true : false;
        if (ImGui::MenuItem("Load trajectories...", NULL, nullptr, enable))
            ui.mWindows["openTraj"]->show();

        if (ImGui::MenuItem("Save as ...", NULL, nullptr, enable))
        {
            FileDialog *dialog = reinterpret_cast<FileDialog *>(ui.mWindows["dialog"].get());
            dialog->createDialog(DIALOG_SAVE, "Save as...", {".hdf", ".json"});

            void (*func)(const String &, UI *) = &saveData;
            dialog->callback(func, &ui);
        }

        if (ImGui::MenuItem("Exit"))
            ui.closeApp();

        ImGui::EndMenu();
    } // file-menu

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    if (ImGui::BeginMenu("About"))
    {
        if (ImGui::MenuItem("View inbox messages"))
            ui.mWindows["inbox"]->show();

        if (ImGui::MenuItem("How to cite"))
        {
            ui.mail.createMessage<MSG_Info>("When the paper is out");
            ui.mWindows["inbox"]->show();
        }

        if (ImGui::MenuItem("Documentation"))
        {
            ui.mail.createMessage<MSG_Info>("Link to github later");
            ui.mWindows["inbox"]->show();
        }

        ImGui::EndMenu();
    }

} // ImGuiMenuBarLayer

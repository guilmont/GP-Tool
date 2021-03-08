#include "childWindows.h"

// HELPER FUNCTIONS
static void drawTableWidget(const char **names, const MatrixXd &mat)
{
    const uint32_t nRows = mat.rows(), nCols = mat.cols();

    ImVec2 region = ImGui::GetContentRegionAvail();

    ImGui::BeginChild("child", region, true);

    // HEADER
    ImGui::Separator();
    ImGui::Columns(nCols);

    for (uint32_t k = 0; k < nCols; k++)
    {
        ImGui::Text("%s", names[k]);
        ImGui::NextColumn();
    }
    ImGui::Columns(1);

    ImGui::Separator();

    ImGui::Columns(nCols);
    // MAIN BODY
    for (uint32_t k = 0; k < nRows; k++)
    {
        char label[32];
        sprintf(label, "%d", int(mat(k, 0)));
        ImGui::Selectable(label, false, ImGuiSelectableFlags_SpanAllColumns);
        ImGui::NextColumn();

        for (uint32_t l = 1; l < nCols; l++)
        {
            ImGui::Text("%.4f", mat(k, l));
            ImGui::NextColumn();
        }

    } // loop-rows

    ImGui::Columns(1);
    ImGui::Separator();

    ImGui::EndChild();

} // draWTableWidtget

static void plotDistribution(const MatrixXd &mat, const uint32_t col,
                             const char *title, const char *xLabel,
                             const ImVec2 &size, const ImVec4 &color, double calib = 1.0)
{
    // Build steps for histogram plot
    const uint32_t N = 2 * mat.rows() - 1;
    MatrixXd xy(N, 2);
    for (uint32_t k = 0; k < mat.rows() - 1; k++)
    {
        xy.row(2 * k) << calib * mat(k, col), mat(k, col + 1);
        xy.row(2 * k + 1) << calib * mat(k + 1, col), mat(k, col + 1);
    }

    xy.row(N - 1) << calib * mat(mat.rows() - 1, col + 0), mat(mat.rows() - 1, col + 1);

    // Calculatin x and y limits for plot
    double xmin = 0.8 * xy.col(0).minCoeff(), xmax = 1.2 * xy.col(0).maxCoeff();
    double ymin = 0.0, ymax = 1.2 * xy.col(1).maxCoeff();
    ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax, ImGuiCond_Once);

    ImGui::PushID(std::to_string(rand()).c_str());
    if (ImPlot::BeginPlot(title, xLabel, "Density", size, ImPlotAxisFlags_Lock))
    {
        ImPlot::PushStyleColor(ImPlotCol_Fill, color);
        ImPlot::PlotShaded("##", xy.col(0).data(), xy.col(1).data(), xy.rows(), 0.0);
        ImPlot::PopStyleColor();

        ImPlot::EndPlot();
    }
    ImGui::PopID();

} // calcSteps

static void saveCSV(const String &path, std::pair<const MatrixXd *, UI *> info)
{
    auto [mat, ui] = info;

    const char *names[] = {"Frame", "Time", "Position X", "Error X",
                           "Position Y", "Error Y", "Size X", "Size Y",
                           "signal", "Background"};

    const uint32_t nCols = mat->cols(),
                   nRows = mat->rows();

    std::ofstream arq(path);
    // Header
    for (uint32_t l = 0; l < nCols; l++)
        arq << names[l] << (l == nCols - 1 ? "\n" : ", ");

    // Body
    for (uint32_t k = 0; k < nRows; k++)
        for (uint32_t l = 0; l < nCols; l++)
            arq << (*mat)(k, l) << (l == nCols - 1 ? "\n" : ", ");

    arq.close();

    ui->mail.createMessage<MSG_Info>("Data saved to " + path);
    ui->mWindows["inbox"]->show();

} // saveCSV

/*****************************************************************************/
/*****************************************************************************/

void winOpenTrajectory::display(void)
{
    UI &ui = *reinterpret_cast<UI *>(user_data);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());
    auto enhanceTraj = [](MainStruct *data) -> void {
        data->track->enhanceTracks();
        data->updateViewport_flag = true;
    };

    auto uploadTrack = [&](const String &path, uint32_t channel) -> void {
        bool check;
        if (path.find(".xml") != String::npos)
            check = data.track->useICY(path, channel);
        else
            check = data.track->useCSV(path, channel);

        if (!check)
        {
            ui.mWindows["inbox"]->show();
            this->hide();
            data.track = nullptr;
        }
    };

    if (!data.track)
        data.track = std::make_unique<Trajectory>(data.movie.get(), &ui.mail);

    ImGui::Begin("Select Trajectories...");

    const uint32_t nChannels = data.movie->getMetadata().SizeC;
    const float width = ImGui::GetContentRegionAvailWidth();

    for (uint32_t ch = 0; ch < nChannels; ch++)
    {
        String txt = "Channel " + std::to_string(ch);
        ImGui::PushID(txt.c_str());

        char buf[1024] = {0};
        ImGui::Text("%s", txt.c_str());
        ImGui::SameLine();

        if (data.track->contains(ch))
        {
            const String &path = data.track->getTrack(ch).path;
            strcpy(buf, path.c_str());
        }

        ImGui::SetNextItemWidth(0.75 * width);
        ImGui::InputText("##input", buf, 1024, ImGuiInputTextFlags_ReadOnly);

        ImGui::SameLine();
        if (ImGui::Button("Open"))
        {
            FileDialog *dialog = reinterpret_cast<FileDialog *>(ui.mWindows["dialog"].get());
            dialog->createDialog(DIALOG_OPEN, "Choose track...", {".xml", ".csv"});
            dialog->callback(uploadTrack, ch);
        }

        ImGui::PopID();

        ImGui::Spacing();
    } // loop channels

    ImGui::Spacing();

    if (ImGui::Button("Done"))
    {
        this->hide();
        ui.mWindows["inbox"]->show();
        std::thread(enhanceTraj, &data).detach();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel"))
    {
        this->hide();
        data.track = nullptr;
    }

    ImGui::End();
} // trajDialog

/*****************************************************************************/
/*****************************************************************************/

void winTrajectory::setInfo(uint32_t channel, uint32_t trajID)
{
    ch = channel;
    id = trajID;
}

void winTrajectory::display(void)
{
    UI *ui = reinterpret_cast<UI *>(user_data);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui->getUserData());

    ImGui::Begin("Trajectory", &active);

    ui->fonts.text("Path: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%s", data.track->getTrack(ch).path.c_str());

    ui->fonts.text("Description: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%s", data.track->getTrack(ch).description.c_str());

    ui->fonts.text("Channel: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", ch);

    ui->fonts.text("Trajectory: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%d", id);

    ui->fonts.text("Length: ", "bold");
    ImGui::SameLine();
    ImGui::Text("%ld points", data.track->getTrack(ch).traj[id].rows());

    ImGui::Spacing();

    if (ImGui::Button("Export"))
    {
        std::pair<const MatrixXd *, UI *> info{&data.track->getTrack(ch).traj[id], ui};

        FileDialog *dialog = (FileDialog *)ui->mWindows["dialog"].get();
        dialog->createDialog(DIALOG_SAVE, "Save CSV file...", {".csv"});
        dialog->callback(saveCSV, info);

        // data.showDialog_flag = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("View plots"))
    {
        winPlotTrajectory *win = (winPlotTrajectory *)ui->mWindows["plotTraj"].get();
        win->setInfo(&data.track->getTrack(ch).traj[id], "Movement");
        win->show();
    }

    ImGui::Spacing();
    ImGui::Spacing();

    MatrixXd mat = data.track->getTrack(ch).traj[id];
    if (data.align && ch > 0)
    {
        MatrixXd trf = data.align->getTransformData(ch).trf;
        for (uint32_t k = 0; k < mat.rows(); k++)
        {
            double posx = mat(k, TrajColumns::POSX),
                   posy = mat(k, TrajColumns::POSY);

            mat(k, TrajColumns::POSX) = trf(0, 0) * posx + trf(0, 1) * posy + trf(0, 2);
            mat(k, TrajColumns::POSY) = trf(1, 0) * posx + trf(1, 1) * posy + trf(1, 2);
        }
    } // if-alignment

    const char *names[] = {"Frame", "Time", "Position X", "Error X",
                           "Position Y", "Error Y", "Size X", "Size Y",
                           "signal", "Background"};

    drawTableWidget(names, mat);

    ImGui::End();
} // winTrajectory

/*****************************************************************************/
/*****************************************************************************/

void winSubstrate::setCellID(uint32_t cellID)
{
    id = cellID;
}

void winSubstrate::display(void)
{
    UI *ui = reinterpret_cast<UI *>(user_data);
    MainStruct &data = *reinterpret_cast<MainStruct *>(ui->getUserData());

    const GP_FBM &gp = data.vecGP[id];

    if (!gp.hasSubstrate())
        return;

    const float
        pix2mu = data.movie->getMetadata().PhysicalSizeXY,
        DCalib = pix2mu * pix2mu;

    const char
        *spaceUnit = data.movie->getMetadata().PhysicalSizeXYUnit.c_str(),
        *timeUnit = data.movie->getMetadata().TimeIncrementUnit.c_str();

    ImGui::Begin("Substrate", &active);

    const ParamCDA &cda = gp.getCoupledParameters();

    ImGui::Text("Optimal values:");

    ImGui::Text("  DR: %.3e %s^2/%s^A", DCalib * cda.DR, spaceUnit, timeUnit);
    ImGui::Text("  AR: %.3f", cda.AR);
    ImGui::Spacing();

    if (gp.hasDistribCoupled())
    {
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::Text("COEFFICIENTS DISTRIBUTION");
        ImGui::Spacing();
        const MatrixXd &mat = data.vecGP[id].getCoupledDistribution();

        float avail = ImGui::GetContentRegionAvailWidth();
        const ImVec2 size(0.5 * avail, 0.33 * avail);
        const uint32_t pid = mat.cols() - 4;

        plotDistribution(mat, pid, "Diffusion coefficient", "D ( x 0.001)", size,
                         g_color::green, DCalib * 1000.0);

        ImGui::SameLine();
        plotDistribution(mat, pid + 2, "Constrainment", "Alpha", size,
                         g_color::red);

        ImGui::Spacing();
        ImGui::Spacing();
    } // if-distribution coupled

    ImGui::Separator();

    ImGui::Spacing();
    ImGui::Text("SUBSTRATE MOVEMENT");
    ImGui::SameLine();
    if (ImGui::Button("View plot"))
    {
        winPlotTrajectory *win = (winPlotTrajectory *)ui->mWindows["plotTraj"].get();
        win->setInfo(&data.vecGP[id].getSubstrate(), "Movement");
        win->show();
    }

    ImGui::SameLine();
    if (ImGui::Button("Export"))
    {
        std::pair<const MatrixXd *, UI *> info{&data.vecGP[id].getSubstrate(), ui};

        FileDialog *dialog = (FileDialog *)ui->mWindows["dialog"].get();
        dialog->createDialog(DIALOG_SAVE, "Save CSV file...", {".csv"});
        dialog->callback(saveCSV, info);
    }

    ImGui::Spacing();

    const MatrixXd &mat = data.vecGP[id].getSubstrate();
    const char *name[] = {"Frame", "Time", "Position X", "Error X",
                          "Position Y", "Error Y"};

    drawTableWidget(name, mat);

    ImGui::End();

} // winSubstrate

/*****************************************************************************/
/*****************************************************************************/

void winPlotTrajectory::setInfo(const MatrixXd *mat, const String &current)
{
    this->mat = mat;
    this->current = current;
}

void winPlotTrajectory::display(void)
{
    // Generic function to plot any trajectory

    ImGui::Begin("Trajectory plots", &active);

    const uint32_t nPlots = mat->cols() > 6 ? 3 : 1;

    if (nPlots > 1)
    {
        const char *name[] = {"Movement", "Size", "Signal"};

        if (ImGui::BeginCombo("Choose", current.c_str()))
        {
            for (uint32_t k = 0; k < nPlots; k++)
                if (ImGui::Selectable(name[k]))
                {
                    current = String(name[k]);
                    ImGui::SetItemDefaultFocus();
                } // if-selected

            ImGui::EndCombo();
        }
    } // if-choice

    const float avail = ImGui::GetContentRegionAvailWidth();
    const ImVec2 size = {avail, 0.6f * avail};

    const uint32_t N = mat->rows();
    const double xmin = mat->operator()(0, 0),
                 xmax = mat->operator()(mat->rows() - 1, 0);

    const VectorXd &frame = mat->col(TrajColumns::FRAME);

    if (current.compare("Movement") == 0)
    {
        // TODO: Correct alignment
        VectorXd X = mat->col(TrajColumns::POSX),
                 Y = mat->col(TrajColumns::POSY);

        X.array() -= X(0);
        Y.array() -= Y(0);

        VectorXd errx = 1.96 * mat->col(TrajColumns::ERRX),
                 erry = 1.96 * mat->col(TrajColumns::ERRY);

        VectorXd lowX = X - errx, highX = X + errx;
        VectorXd lowY = Y - erry, highY = Y + erry;

        const double ymin = std::min(lowX.minCoeff(), lowY.minCoeff()),
                     ymax = std::max(highX.maxCoeff(), highY.maxCoeff());

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax);

        if (ImPlot::BeginPlot("Movement", "Frame", "Position", size))
        {
            ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.5f);
            ImPlot::PlotShaded("X-Axis", frame.data(), lowX.data(), highX.data(), N);
            ImPlot::PlotLine("X-Axis", frame.data(), X.data(), N);

            ImPlot::PlotShaded("Y-Axis", frame.data(), lowY.data(), highY.data(), N);
            ImPlot::PlotLine("Y-Axis", frame.data(), Y.data(), N);
            ImPlot::PopStyleVar();

            ImPlot::EndPlot();
        }
    } // if-movement

    else if (current.compare("Size") == 0)
    {
        const VectorXd &szx = mat->col(TrajColumns::SIZEX),
                       &szy = mat->col(TrajColumns::SIZEY);

        const double ymin = 0.5 * std::min(szx.minCoeff(), szy.minCoeff()),
                     ymax = 1.2 * std::max(szx.maxCoeff(), szy.maxCoeff());

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax);

        if (ImPlot::BeginPlot("Spot Size", "Frame", "Size", size))
        {
            ImPlot::PlotLine("X-Axis", frame.data(), szx.data(), N);
            ImPlot::PlotLine("Y-Axis", frame.data(), szy.data(), N);
            ImPlot::EndPlot();
        }
    } // if-size

    else
    {
        const VectorXd &bg = mat->col(TrajColumns::BACKGROUND),
                       signal = mat->col(TrajColumns::INTENSITY) + bg;

        const double ymin = 0.5 * std::min(bg.minCoeff(), signal.minCoeff()),
                     ymax = 1.2 * std::max(bg.maxCoeff(), signal.maxCoeff());

        ImPlot::SetNextPlotLimits(xmin, xmax, ymin, ymax);

        if (ImPlot::BeginPlot("Signal intensity", "Frame", "Signal", size))
        {
            ImPlot::PlotLine("Spot", mat->col(0).data(), signal.data(), N);
            ImPlot::PlotLine("Background", mat->col(0).data(), bg.data(), N);
            ImPlot::EndPlot();
        }
    } // else-signal

    ImGui::End();
} // plotTrajectory

/*****************************************************************************/
/*****************************************************************************/

void winPlotDistribution::setCellID(uint32_t cellID)
{
    id = cellID;
}

void winPlotDistribution::display(void)
{
    MainStruct &data = *reinterpret_cast<MainStruct *>(user_data);

    const GP_FBM &gp = data.vecGP[id];

    // No distributions calculated yet
    if (!gp.hasDistribSingle() && !gp.hasDistribCoupled())
        return;

    const double calib = data.movie->getMetadata().PhysicalSizeXY;

    ImGui::Begin("Distributions", &active);

    uint32_t nCols;
    const MatrixXd *mat = nullptr;
    if (data.vecGP[id].getNumParticles() > 1)
    {
        mat = &data.vecGP[id].getCoupledDistribution();
        nCols = mat->cols() - 4;
    }
    else
    {
        mat = &data.vecGP[id].getSingleDistribution()[0];
        nCols = 4;
    }

    float avail = ImGui::GetContentRegionAvailWidth();
    const ImVec2 size(0.5 * avail, 0.33 * avail);

    for (uint32_t k = 0; k < nCols; k += 4)
    {
        ImGui::Text("Particle %d", int(0.25 * k));
        ImGui::Spacing();

        plotDistribution(*mat, k, "Diffusion coefficient", "D (x 0.001)", size,
                         g_color::green, calib * calib * 1000);
        ImGui::SameLine();
        plotDistribution(*mat, k + 2, "Constrainment", "Alpha", size, g_color::red);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    ImGui::End();
} // winDistribution
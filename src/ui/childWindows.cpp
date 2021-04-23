#include "childWindows.h"

// HELPER FUNCTIONS

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

        plotDistribution(*mat, k, "Diffusion coefficient", "D", size,
                         g_color::green, calib * calib);
        ImGui::SameLine();
        plotDistribution(*mat, k + 2, "Anomalous coefficient", "Alpha", size, g_color::red);
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }

    ImGui::End();
} // winDistribution
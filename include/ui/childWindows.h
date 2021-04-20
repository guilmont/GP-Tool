#pragma once

#include "window.h"
#include "ui.h"
#include "structure.hpp"

#include <implot.h>

class winOpenTrajectory : public Window
{
public:
    winOpenTrajectory(void *ui) { user_data = ui; }
    void display(void) override;
};

/////////////////////////////

class winTrajectory : public Window
{
public:
    winTrajectory(void *ui) { user_data = ui; }
    void display(void) override;

    void setInfo(uint32_t channel, uint32_t trajID);

private:
    uint32_t ch, id;
};

/////////////////////////////

class winSubstrate : public Window
{
public:
    winSubstrate(void *ui) { user_data = ui; }
    void display(void) override;

    void setCellID(uint32_t cellID);

private:
    uint32_t id;
};

/////////////////////////////

class winPlotTrajectory : public Window
{
public:
    winPlotTrajectory(void) = default;
    void display(void) override;

    void setInfo(const MatrixXd *mat, const String &current);

private:
    const MatrixXd *mat = nullptr;
    String current;
};

/////////////////////////////

class winPlotDistribution : public Window
{
public:
    winPlotDistribution(void *data) { user_data = data; }
    void display(void) override;

    void setCellID(uint32_t cellID);

private:
    uint32_t id;
};
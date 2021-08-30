#pragma once

#include "plugin.h"
#include "gp_fbm.h"

#include "gptool.h"
class GPTool;

class GPPlugin : public Plugin
{
public:
    GPPlugin(GPTool *ptr);
    ~GPPlugin(void);

    void showWindows(void) override;
    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;

private: 

    // display windows
    struct
    {
        bool show = false;
        uint64_t gpID = 0, trajID = 0;

    } avgView;

    struct
    {
        bool show = false;
        uint64_t gpID = 0;
    } subPlotView, subView, distribView;

    void winAvgView(void);
    void winSubstrate(void);
    void winPlotSubstrate(void);
    void winDistributions(void);

    void addNewCell(const std::vector<MatXd>& vTraj, const std::vector<GPT::GP_FBM::ParticleID>& partID);

private:
    GPTool *tool = nullptr;

    std::vector<std::unique_ptr<GPT::GP_FBM>> vecGP;
};
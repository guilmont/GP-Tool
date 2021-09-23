#pragma once

#include "plugin.h"
#include "gp_fbm.h"

#include "gptool.h"
class GPTool;


struct GP2Show
{
    std::unique_ptr<GPT::GP_FBM> gp = nullptr;

    std::vector<std::unique_ptr<MatXd>> average;
    
    std::unique_ptr<MatXd> distribSingle = nullptr;
    std::unique_ptr<MatXd> distribCouple = nullptr;
    std::unique_ptr<MatXd> substrate = nullptr;
};



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

    std::vector<GP2Show> vecGP;
};
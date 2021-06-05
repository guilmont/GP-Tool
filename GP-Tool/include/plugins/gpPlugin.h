#pragma once

#include "plugin.h"
#include "methods/gp_fbm.h"

#include "gptool.h"

class GPPlugin : public Plugin
{
public:
    GPPlugin(GPTool *ptr);
    ~GPPlugin(void);

    void showWindows(void) override;
    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;

private: // Windows
    struct
    {
        bool show = false;
        uint32_t gpID, trajID;

    } avgView;

    struct
    {
        bool show = false;
        uint32_t gpID;
    } subPlotView, subView, distribView;

    void winAvgView(void);
    void winSubstrate(void);
    void winPlotSubstrate(void);
    void winDistributions(void);

private:
    GPTool *tool = nullptr;

    std::vector<std::unique_ptr<GP_FBM>> vecGP;
};
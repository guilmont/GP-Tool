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

private: // Windows
private:
    GPTool *tool = nullptr;

    std::vector<std::unique_ptr<GP_FBM>> vecGP;
};
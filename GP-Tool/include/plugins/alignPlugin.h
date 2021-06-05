#pragma once

#include "plugin.h"
#include "gptool.h"

#include "methods/align.h"

class AlignPlugin : public Plugin
{
public:
    AlignPlugin(const Movie *mov, GPTool *ptr);
    ~AlignPlugin(void);

    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;

    std::vector<TransformData> data;

private:
    const Movie *movie = nullptr;
    GPTool *tool = nullptr;

    uint32_t chAlign = 1;
    bool camera = true,
         chromatic = true;

    void runAlignment(void);
};
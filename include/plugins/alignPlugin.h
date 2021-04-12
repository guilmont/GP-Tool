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

    std::vector<TransformData> data;

private:
    const Movie *movie = nullptr;
    GPTool *tool = nullptr;

    bool camera = true,
         chromatic = true;

    uint32_t chAlign = 1;

    void runAlignment(void);
};
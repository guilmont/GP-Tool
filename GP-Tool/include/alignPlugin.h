#pragma once

#include "plugin.h"
#include "movie.h"
#include "align.h"

#include "gptool.h"
class GPTool;


class AlignPlugin : public Plugin
{
public:
    AlignPlugin(GPT::Movie *mov, GPTool *ptr);
    ~AlignPlugin(void);

    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;
    void saveTIF(const fs::path& path);
    std::vector<GPT::TransformData> data;

private:
    GPT::Movie *movie = nullptr;
    GPTool *tool = nullptr;

    std::unique_ptr<GPT::Align> m_align = nullptr;

    uint64_t chAlign = 1;
    bool camera = true,
         chromatic = true;

    bool working = false;  // to avoid running over multiple instances

    void runAlignment(void);
};
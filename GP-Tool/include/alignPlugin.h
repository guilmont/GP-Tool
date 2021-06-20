#pragma once

#include "plugin.h"
#include "movie.h"
#include "align.h"

#include "gptool.h"
class GPTool;


class AlignPlugin : public Plugin
{
public:
    AlignPlugin(Movie *mov, GPTool *ptr);
    ~AlignPlugin(void);

    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;

    std::vector<TransformData> data;

private:
    Movie *movie = nullptr;
    GPTool *tool = nullptr;

    std::unique_ptr<Align> m_align = nullptr;

    uint32_t chAlign = 1;
    bool camera = true,
         chromatic = true;

    bool working = false;  // to avoid running over multiple instances

    void runAlignment(void);
};
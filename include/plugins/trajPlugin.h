#pragma once

#include "plugin.h"
#include "methods/trajectory.h"

#include "gptool.h"

struct UITraj
{
    uint32_t trackID, trajID;
    bool display = true;
    glm::vec3 color;
};

class TrajPlugin : public Plugin
{
public:
    TrajPlugin(const Movie *mov, GPTool *ptr);
    ~TrajPlugin(void);

    void showWindows(void) override;
    void showProperties(void) override;
    void update(float deltaTime) override;

    void loadTracks(void) { trackInfo.show = true; }

private:
    struct
    {
        bool show = false;
        int32_t spotSize = 3, openCH = 0;
        std::vector<std::string> path;
    } trackInfo;

    void winLoadTracks(void);

private:
    const Movie *movie = nullptr;
    GPTool *tool = nullptr;

    std::unique_ptr<Trajectory> m_traj = nullptr;

    void enhanceTracks(void);
};

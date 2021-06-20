#pragma once

#include "plugin.h"
#include "trajectory.h"

#include "gptool.h"
class GPTool;

using UITraj = std::vector<std::pair<bool, glm::vec3>>;

class TrajPlugin : public Plugin
{
public:
    TrajPlugin(Movie *mov, GPTool *ptr);
    ~TrajPlugin(void);

    void showWindows(void) override;
    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;

    void loadTracks(void) { trackInfo.show = true; }

    const Trajectory *getTrajectory(void) { return m_traj.get(); }
    UITraj *getUITrajectory(void) { return uitraj; }

private:
    struct
    {
        bool show = false;
        int32_t spotSize = 3, openCH = 0;
        std::vector<fs::path> path;
    } trackInfo;

    struct
    {
        bool show = false;
        uint32_t trackID, trajID;
    } detail;

    struct
    {
        bool show = false;
        uint32_t trackID, trajID, plotID = 0;
        const char *options[3] = {"Movement", "Spot size", "Signal"};
    } plot;

    void winLoadTracks(void);
    void winDetail(void);
    void winPlots(void);

private:
    Movie *movie = nullptr;
    GPTool *tool = nullptr;

    std::unique_ptr<Trajectory> m_traj = nullptr;

    UITraj *uitraj = nullptr;

    void enhanceTracks(void);
};

#pragma once

#include "plugin.h"
#include "trajectory.h"

#include "circle.h"
#include "roi.h"

#include "gptool.h"
class GPTool;

using UITraj = std::vector<std::pair<bool, glm::vec4>>;


class TrajPlugin : public Plugin
{
public:
    TrajPlugin(GPT::Movie *mov, GPTool *ptr);
    ~TrajPlugin(void);

    void showWindows(void) override;
    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;
    bool isActive(void) override { return m_traj != nullptr; }

    void loadTracks(void) { trackInfo.show = true; }

    const GPT::Trajectory *getTrajectory(void) { return m_traj.get(); }
    UITraj *getUITrajectory(void) { return uitraj; }

    Roi roi;
    
private:
    struct
    {
        bool show = false;
        uint64_t spotSize = 3, openCH = 0;
        std::vector<fs::path> path;
    } trackInfo;

    struct
    {
        bool show = false;
        uint64_t trackID = 0, trajID = 0;
    } detail;

    struct
    {
        bool show = false;
        uint64_t trackID = 0, trajID = 0, plotID = 0;
        const char *options[3] = {"Movement", "Spot size", "Signal"};
    } plot;

    void winLoadTracks(void);
    void winDetail(void);
    void winPlots(void);

private:
    GPT::Movie *movie = nullptr;
    GPTool *tool = nullptr;

    std::unique_ptr<GPT::Trajectory> m_traj = nullptr;

    uint64_t maxSpots = 512; // I don't think we need more than that per frame

    std::unique_ptr<Circle> m_circle = nullptr;
    std::unique_ptr<GRender::Quad> m_quad= nullptr;

    UITraj *uitraj = nullptr;

    void enhanceTracks(void);
};

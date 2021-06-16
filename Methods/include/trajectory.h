#pragma once

#include "header.h"
#include "movie.h"
#include "spot.h"

struct Track
{
    std::string path, description = "No description";

    std::vector<MatXd> traj;

    enum : const uint32_t
    {
        FRAME = 0,
        TIME = 1,
        POSX = 2,
        POSY = 3,
        ERRX = 4,
        ERRY = 5,
        SIZEX = 6,
        SIZEY = 7,
        BG = 8,
        SIGNAL = 9,
        NCOLS = 10
    };
};

class Trajectory
{
public:
    GP_API Trajectory(const Movie *mov);
    GP_API ~Trajectory(void) = default;

    uint32_t spotSize = 3;
    GP_API  bool useICY(const std::string &xmlTrack, uint32_t ch = 0);
    GP_API bool useCSV(const std::string &csvTrack, uint32_t ch = 0);
    GP_API void enhanceTracks(void);

    GP_API uint32_t getNumTracks(void) const { return uint32_t(m_vTrack.size()); }

    GP_API Track &getTrack(const uint32_t ch = 0) { return m_vTrack[ch]; }
    GP_API const Track &getTrack(const uint32_t ch = 0) const { return m_vTrack.at(ch); }

private:
    void enhancePoint(uint32_t trackID, uint32_t trajID, uint32_t tid);
    void enhanceTrajectory(uint32_t trackID, uint32_t trajID);

private:
    const Movie *movie = nullptr;
    std::vector<Track> m_vTrack;

};

#pragma once

#include "header.h"
#include "movie.h"
#include "spot.h"

struct Track
{
    uint32_t channel;
    std::string path;
    std::string description = "No description";

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
    Trajectory(const Movie *mov, Mailbox *mail = nullptr);
    ~Trajectory(void);

    uint32_t spotSize = 3;
    bool useICY(const std::string &xmlTrack, uint32_t ch = 0);
    bool useCSV(const std::string &csvTrack, uint32_t ch = 0);
    void enhanceTracks(void);

    uint32_t getNumTracks(void) const { return uint32_t(m_vTrack.size()); }

    Track &getTrack(const uint32_t ch = 0) { return m_vTrack[ch]; }
    const Track &getTrack(const uint32_t ch = 0) const { return m_vTrack.at(ch); }

private:
    void enhancePoint(uint32_t trackID, uint32_t trajID, uint32_t tid);
    void enhanceTrajectory(uint32_t trackID, uint32_t trajID);

private:
    const Movie *movie = nullptr;
    Mailbox *mbox = nullptr;

    std::vector<Track> m_vTrack;

}; // class Trajectory

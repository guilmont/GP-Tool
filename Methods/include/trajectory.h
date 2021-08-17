#pragma once

#include "header.h"
#include "movie.h"
#include "spot.h"

namespace GPT
{

    struct Track
    {
        fs::path path;
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
        GP_API Trajectory(Movie *mov);
        GP_API ~Trajectory(void) = default;

        uint32_t spotSize = 3;
        GP_API  bool useICY(const fs::path &xmlTrack, uint32_t ch = 0);
        GP_API bool useCSV(const fs::path &csvTrack, uint32_t ch = 0);
        GP_API void enhanceTracks(void);
        
        GP_API float getProgress(void) const { return progress;  }
        GP_API void stop(void) { running = false; }

        GP_API uint32_t getNumTracks(void) const { return uint32_t(m_vTrack.size()); }

        GP_API Track &getTrack(const uint32_t ch = 0) { return m_vTrack[ch]; }
        GP_API const Track &getTrack(const uint32_t ch = 0) const { return m_vTrack.at(ch); }

    private:
        void enhancePoint(uint32_t trackID, uint32_t trajID, uint32_t tid);
        void enhanceTrajectory(uint32_t trackID, uint32_t trajID);

    private:
        Movie *movie = nullptr;
        std::vector<Track> m_vTrack;

        bool running = false;
        float progress = 0.0f;

    };

}
#pragma once

#include "header.h"
#include "movie.h"
#include "spot.h"

namespace GPT
{

    struct Track_API
    {
        fs::path path;
        std::string description = "No description";

        std::vector<MatXd> traj;
              
    };

    class Trajectory
    {
    public:
        GP_API Trajectory(Movie *mov);
        GP_API ~Trajectory(void) = default;

        uint64_t spotSize = 3;
        GP_API  bool useICY(const fs::path &xmlTrack, uint64_t ch = 0);
        GP_API bool useCSV(const fs::path &csvTrack, uint64_t ch = 0);
        GP_API void enhanceTracks(void);
        
        GP_API float getProgress(void) const { return progress;  }
        GP_API void stop(void) { running = false; }

        GP_API uint64_t getNumTracks(void) const { return uint64_t(m_vTrack.size()); }

        GP_API Track_API &getTrack(const uint64_t ch = 0) { return m_vTrack[ch]; }
        GP_API const Track_API &getTrack(const uint64_t ch = 0) const { return m_vTrack.at(ch); }

    private:
        void enhancePoint(uint64_t trackID, uint64_t trajID, uint64_t tid, uint64_t nThreads);
        void enhanceTrajectory(uint64_t trackID, uint64_t trajID);

    private:
        Movie *movie = nullptr;
        std::vector<Track_API> m_vTrack;

        bool running = false;
        float progress = 0.0f;

    };

}
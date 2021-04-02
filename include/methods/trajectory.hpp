#pragma once

#include <config.hpp>
#include <movie.hpp>
#include <spot.hpp>
#include <array>
#include <map>

using View = std::pair<bool, std::array<float, 3>>;

struct Track
{
    uint32_t channel;
    std::string path;
    std::string description = "No description";

    std::vector<View> view;
    std::vector<MatrixXd> traj;
    MatrixXd *vImage = nullptr;
};

class Trajectory
{
    friend void transferEnhance(const uint32_t, void *);

private:
    uint32_t m_spotRadius = 3;

    Movie *movie = nullptr;
    std::map<uint32_t, Track> m_vTrack;
    Mailbox *mail;

public:
    Trajectory(Movie *mov, Mailbox *mailbox) : movie(mov), mail(mailbox) {}
    Trajectory(Trajectory &&obj);
    ~Trajectory(void);

    bool useICY(const std::string &xmlTrack, uint32_t ch = 0);
    bool useCSV(const std::string &csvTrack, uint32_t ch = 0);
    void enhanceTracks(void);

    inline void setSpotRadius(uint32_t radius) { m_spotRadius = radius; }
    uint32_t getSpotRadius(void) const { return m_spotRadius; }

    Track &getTrack(const uint32_t ch = 0) { return m_vTrack[ch]; }
    const Track &getTrack(const uint32_t ch = 0) const { return m_vTrack.at(ch); }

    uint32_t getNumTracks(void) const { return m_vTrack.size(); }
    bool contains(const uint32_t ch) { return m_vTrack.find(ch) != m_vTrack.end(); }

}; // class Trajectory

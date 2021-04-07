#pragma once

#include "header.h"
#include "metadata.h"

class Movie
{

public:
    Movie(const std::string &movie_path, Mailbox *mail = nullptr);
    ~Movie(void);

    bool successful(void) const { return success; }

    const Metadata &getMetadata(void) const { return *(meta.get()); }
    Metadata &getMetadata(void) { return *(meta.get()); }

    MatrixXd &getImage(uint32_t channel, uint32_t frame);
    MatrixXd *getChannel(uint32_t channel);

private:
    Mailbox *mbox = nullptr;
    bool success = true;

    MatrixXd **vImg = nullptr;
    std::unique_ptr<Metadata> meta = nullptr;

}; // class Trajectory

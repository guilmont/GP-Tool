#pragma once

#include "header.h"
#include "metadata.h"

class Movie
{

public:
    Movie(const std::string &movie_path, Mailbox *mail = nullptr);
    Movie(void) = default;
    ~Movie(void) = default;

    bool successful(void) const { return success; }

    const Metadata &getMetadata(void) const { return meta; }
    Metadata &getMetadata(void) { return meta; }

    MatrixXd &getImage(uint32_t channel, uint32_t frame);
    MatrixXd *getChannel(uint32_t frame);

private:
    Mailbox *mbox = nullptr;
    bool success = true;

    Metadata meta;
    std::vector<MatrixXd> vImg;

}; // class Trajectory

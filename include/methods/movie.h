#pragma once

#include "header.h"
#include "metadata.h"

class Movie
{

public:
    API Movie(const std::string &movie_path, Mailbox *mail = nullptr);
    API Movie(void) = default;
    API ~Movie(void) = default;

    API bool successful(void) const { return success; }

    API const Metadata &getMetadata(void) const { return meta; }
    API Metadata &getMetadata(void) { return meta; }

    API const MatXd &getImage(uint32_t channel, uint32_t frame) const;

private:
    Mailbox *mbox = nullptr;
    bool success = true;

    Metadata meta;
    std::vector<MatXd> vImg;

}; // class Trajectory

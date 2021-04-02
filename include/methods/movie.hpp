#pragma once

#include <config.hpp>
#include <tiffer.hpp>
#include <metadata.hpp>

class Movie
{
private:
    Metadata meta;
    MatrixXd **vImg = nullptr;
    Mailbox *mail = nullptr;

public:
    Movie(Mailbox *mailbox) : mail(mailbox) {}
    Movie(Movie &&movie);
    ~Movie(void);

    bool loadMovie(const std::string &filename);

    const Metadata &getMetadata(void) const { return meta; }
    Metadata &getMetadata(void) { return meta; }

    MatrixXd &getImage(uint32_t channel, uint32_t frame);
    MatrixXd *getChannel(uint32_t channel);

}; // class Trajectory

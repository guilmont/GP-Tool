#pragma once

#include "header.h"

#include "gtiffer.h"
#include "metadata.h"

class Movie
{

public:
    GP_API Movie(const std::string &movie_path);
    GP_API ~Movie(void) = default;

    GP_API bool successful(void) const { return success; }

    GP_API const Metadata &getMetadata(void) const;
    GP_API Metadata &getMetadata(void);

    GP_API const MatXd& getImage(uint32_t channel, uint32_t frame);

private:
    bool success = true;

    std::unique_ptr<Metadata> meta = nullptr;

    // We are going to setup for lazy loading
    std::unique_ptr<Tiffer::Read> tif = nullptr;
    std::vector<std::unique_ptr<MatXd>> vImg;

}; // class Trajectory

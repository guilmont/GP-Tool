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

    GP_API const Metadata& getMetadata(void) const;
    GP_API Metadata& getMetadata(void);

    GP_API const MatXd &getImage(uint32_t channel, uint32_t frame) const;

private:
    bool success = true;

    std::unique_ptr<Metadata> meta = nullptr;
    std::vector<MatXd> vImg;

}; // class Trajectory

#pragma once

#include "header.h"

#include "gtiffer.h"
#include "metadata.h"

namespace GPT
{
    class Movie
    {

    public:
        GP_API Movie(const fs::path &movie_path);
        GP_API ~Movie(void) = default;

        GP_API bool successful(void) const { return success; }

        GP_API const Metadata &getMetadata(void) const;
        GP_API Metadata &getMetadata(void);

        GP_API const MatXd& getImage(uint64_t channel, uint64_t frame);

    private:
        bool success = true;

        std::unique_ptr<Metadata> meta = nullptr;

        // We are going to setup for lazy loading
        std::unique_ptr<Tiffer::Read> tif = nullptr;
        std::vector<MatXd> vImg;

    };
}
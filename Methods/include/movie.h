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

        GP_API const MatXd& getImage(uint32_t channel, uint32_t frame);

        template<typename TP>
        void save(const std::vector<Image<TP>>& vImg, const fs::path& path, bool lzw = false);

    private:
        bool success = true;

        std::unique_ptr<Metadata> meta = nullptr;

        // We are going to setup for lazy loading
        std::unique_ptr<Tiffer::Read> tif = nullptr;
        std::vector<std::unique_ptr<MatXd>> vImg;

    }; // class Trajectory

    template <typename TP>
    void Movie::save(const std::vector<Image<TP>>& vImg, const fs::path& path, bool lzw)
    {
        GPT::Tiffer::Write wrt(vImg, this->getMetadata().metaString, lzw);
        wrt.save(path);
    }

}
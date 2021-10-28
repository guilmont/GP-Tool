#include "movie.h"


namespace GPT
{
    Movie::Movie(const fs::path &movie_path)
    {

        tif = std::make_unique<Tiffer::Read>(movie_path);
        if (!tif->successful())
        {
            success = false;
            tif.release();
            return;
        }

        // Parsing metadata to our needs
        meta = std::make_unique<Metadata>(tif.get());

        // Check if axes are correct
        if (meta->DimensionOrder.compare("XYCZT") != 0)
        {
            success = false;
            pout("ERROR (Movie::Movie): Movies has wrong axes format:", meta->DimensionOrder, ":: Expected -> XYCZT ::", movie_path);
            return;
        }

        // Emplacing empty matrices
        for (uint64_t k =0; k < tif->getNumDirectories(); k++)
            vImg.emplace_back(0,0);
    }

    const Metadata &Movie::getMetadata(void) const
    {
        assert(meta != nullptr);
        return *meta.get();
    }

    Metadata &Movie::getMetadata(void)
    {
        assert(meta != nullptr);
        return *meta.get();
    }

    const MatXd &Movie::getImage(uint64_t channel, uint64_t frame)
    {
        assert(channel < meta->SizeC && frame < meta->SizeT);

        uint32_t id = static_cast<uint32_t>(frame * meta->SizeC + channel);

        if (vImg[id].size() > 0)
            return vImg.at(id);
        else {
            if (meta->SignificantBits == 8)
                vImg[id] = tif->getImage<uint8_t>(id).cast<double>();

            else if (meta->SignificantBits == 16)
                vImg[id] = tif->getImage<uint16_t>(id).cast<double>();

            else if (meta->SignificantBits == 32)
                vImg[id] = tif->getImage<uint32_t>(id).cast<double>();

            return vImg[id];
        }
    }


}
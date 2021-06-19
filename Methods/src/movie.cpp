#include "movie.h"

Movie::Movie(const std::string &movie_path)
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
        gpout("ERROR (Movie::Movie): Movies has wrong axes format:", meta->DimensionOrder, ":: Expected -> XYCZT ::", movie_path);
        return;
    }

    vImg.resize(tif->getNumDirectories());
}

GP_API const Metadata &Movie::getMetadata(void) const
{
    assert(meta != nullptr);
    return *meta.get();
}

GP_API Metadata &Movie::getMetadata(void)
{
    assert(meta != nullptr);
    return *meta.get();
}

const MatXd &Movie::getImage(uint32_t channel, uint32_t frame)
{
    assert(channel < meta->SizeC&& frame < meta->SizeT);

    uint32_t id = frame * meta->SizeC + channel;

    if (vImg[id])
        return *vImg.at(id).get();
    else {
        if (meta->SignificantBits == 8)
            vImg[id] = std::make_unique<MatXd>(tif->getImage<uint8_t>(id).cast<double>());

        else if (meta->SignificantBits == 16)
            vImg[id] = std::make_unique<MatXd>(tif->getImage<uint16_t>(id).cast<double>());

        else if (meta->SignificantBits == 32)
            vImg[id] = std::make_unique<MatXd>(tif->getImage<uint32_t>(id).cast<double>());

        return *vImg[id].get();
    }
}


const MatXd& Movie::getImage(uint32_t channel, uint32_t frame) const 
{
    assert(channel < meta->SizeC && frame < meta->SizeT);
    uint32_t id = frame * meta->SizeC + channel;

    assert(vImg[id] != nullptr);

    return *vImg[id].get();

}
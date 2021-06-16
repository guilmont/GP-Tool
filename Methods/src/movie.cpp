#include "movie.h"


Movie::Movie(const std::string &movie_path)
{

    Tiffer::Read tif(movie_path);
    if (!tif.successful())
    {
        success = false;
        return;
    }

    // Parsing metadata to our needs
    meta = std::make_unique<Metadata>(&tif);

    // Check if axes are correct
    if (meta->DimensionOrder.compare("XYCZT") != 0)
    {
        success = false;
        pout("ERROR (Movie::Movie): Movies has wrong axes format:", meta->DimensionOrder, ":: Expected -> XYCZT ::", movie_path);
        return;
    }

  
    uint32_t
        counter = 0,
        nImg = tif.getNumDirectories();

    vImg.resize(meta->SizeC * meta->SizeT);

    for (uint32_t t = 0; t < meta->SizeT; t++)
        for (uint32_t ch = 0; ch < meta->SizeC; ch++)
        {
            if (meta->SignificantBits == 8)
                vImg[t * meta->SizeC + ch] = tif.getImage<uint8_t>(counter).cast<double>();

            else if (meta->SignificantBits == 16)
                vImg[t * meta->SizeC + ch] = tif.getImage<uint16_t>(counter).cast<double>();

            else if (meta->SignificantBits == 32)
                vImg[t * meta->SizeC + ch] = tif.getImage<uint32_t>(counter).cast<double>();

            // Increasing counter
            counter++;

        } // loop-images

} 

GP_API const Metadata& Movie::getMetadata(void) const
{
    assert(meta != nullptr);
    return *meta.get();
}

GP_API Metadata& Movie::getMetadata(void)
{
    assert(meta != nullptr);
    return *meta.get();
}

const MatXd &Movie::getImage(uint32_t channel, uint32_t frame) const
{
    if (channel >= meta->SizeC)
    {
        pout("WARN (Movie::getImage) >> Channel overflow:", meta->movie_name);
        return vImg[0];
    }

    if (frame >= meta->SizeT)
    {
        pout("WARN (Movie::getImage) >> Frame overflow:", meta->movie_name);
        return vImg[channel];
    }

    return vImg[frame * meta->SizeC + channel];
}

#include "methods/movie.h"

#include "utils/gtiffer.h"

Movie::Movie(const std::string &movie_path, Mailbox *mail) : mbox(mail)
{

    Tiffer::Read tif(movie_path, mail);
    if (!tif.successful())
    {
        success = false;
        return;
    }

    // Parsing metadata to our needs
    meta = Metadata(&tif, mbox);

    // Check if axes are correct
    if (meta.DimensionOrder.compare("XYCZT") != 0)
    {
        success = false;
        std::string msg = "Movie has wrong axes format: " + meta.DimensionOrder;
        msg += " :: Expected -> XYCZT";

        if (mbox)
            mbox->create<Message::Warn>(msg);
        else
            std::cerr << "ERROR (Movie::Movie): " << msg << " -> " << movie_path << std::endl;

        return;
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////

    uint32_t
        counter = 0,
        nImg = tif.getNumDirectories();

    Message::Progress *ptr = nullptr;

    if (mbox)
    {
        mbox->create<Message::Info>("Openning  \"" + meta.movie_name + "\"");
        ptr = mbox->create<Message::Progress>("Loading images");
    }

    vImg.resize(meta.SizeC * meta.SizeT);

    for (uint32_t t = 0; t < meta.SizeT; t++)
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            if (meta.SignificantBits == 8)
                vImg[t * meta.SizeC + ch] = tif.getImage<uint8_t>(counter).cast<double>();

            else if (meta.SignificantBits == 16)
                vImg[t * meta.SizeC + ch] = tif.getImage<uint16_t>(counter).cast<double>();

            else if (meta.SignificantBits == 32)
                vImg[t * meta.SizeC + ch] = tif.getImage<uint32_t>(counter).cast<double>();

            if (ptr)
            {
                ptr->progress = ++counter / float(nImg);
                if (ptr->cancelled)
                {
                    success = false;
                    return;
                }
            }
        } // loop-images

} // constructor

const MatXd &Movie::getImage(uint32_t channel, uint32_t frame) const
{
    if (channel >= meta.SizeC)
    {
        if (mbox)
            mbox->create<Message::Warn>("(Movie::getImage) >>  Channel overflow!!");
        else
            std::cout << "WARN (Movie::getImage) >> Channel overflow: " << meta.movie_name
                      << std::endl;

        return vImg[0];
    }

    if (frame >= meta.SizeT)
    {
        if (mbox)
            mbox->create<Message::Warn>("(Movie::getImage) >> Frame overflow!!");
        else
            std::cout << "WARN (Movie::getImage) >> Frame overflow: " << meta.movie_name
                      << std::endl;

        return vImg[channel];
    }

    return vImg[frame * meta.SizeC + channel];
}

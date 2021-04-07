#include "movie.h"

#include "utils/gtiffer.h"

using Data = std::tuple<uint32_t, const MatrixXd *, MatrixXd &>;

Movie::Movie(const std::string &movie_path, Mailbox *mail) : mbox(mail)
{

    Tiffer::Read tif(movie_path, mail);
    if (!tif.successful())
    {
        success = false;
        return;
    }

    // Parsing metadata to our needs
    meta = std::make_unique<Metadata>(&tif, mbox);

    // Check if axes are correct
    if (meta->DimensionOrder.compare("XYCZT") != 0)
    {
        success = false;
        std::string msg = "Movie has wrong axes format: " + meta->DimensionOrder;
        msg += " :: Expected -> XYCZT";

        if (mbox)
            mbox->create<Message::Warn>(msg);
        else
            std::cerr << "ERROR (Movie::Movie): " << msg << " -> " << movie_path << std::endl;

        return;
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////

    vImg = new MatrixXd *[meta->SizeC];
    for (uint32_t ch = 0; ch < meta->SizeC; ch++)
        vImg[ch] = new MatrixXd[meta->SizeT];

    uint32_t
        counter = 0,
        nImg = tif.getNumDirectories();

    Message::Progress *ptr = mbox->create<Message::Progress>("Loading images");

    ptr->progress = 0.0f;

    for (uint32_t t = 0; t < meta->SizeT; t++)
        for (uint32_t ch = 0; ch < meta->SizeC; ch++)
        {
            if (meta->SignificantBits == 8)
                vImg[ch][t] = tif.getImage<uint8_t>(counter).cast<double>();

            else if (meta->SignificantBits == 16)
                vImg[ch][t] = tif.getImage<uint16_t>(counter).cast<double>();

            else if (meta->SignificantBits == 32)
                vImg[ch][t] = tif.getImage<uint32_t>(counter).cast<double>();

            ptr->progress = ++counter / float(nImg);

            if (ptr->cancel)
            {
                success = false;
                return;
            }

        } // loop-images

} // constructor

Movie::~Movie(void)
{
    for (uint32_t ch = 0; ch < meta->SizeC; ch++)
        delete[] vImg[ch];

    delete[] vImg;
} // destructor

// MatrixXd &Movie::getImage(uint32_t channel, uint32_t frame)
// {
//     if (channel >= meta.SizeC)
//     {
//         mail->createMessage<MSG_Error>(
//             "ERROR: 'getImage' >> channel is greater than available!!");

//         return vImg[0][0];
//     }

//     if (frame >= meta.SizeT)
//     {
//         mail->createMessage<MSG_Error>(
//             "ERROR: 'getImage' >> frame is greater than available!!");

//         return vImg[0][0];
//     }

//     return vImg[channel][frame];
// }

// MatrixXd *Movie::getChannel(uint32_t channel)
// {
//     if (channel >= meta.SizeC)
//     {
//         mail->createMessage<MSG_Error>(
//             "ERROR: 'getImage' >> channel is greater than available!!");
//         return vImg[0];
//     }

//     return vImg[channel];
// }
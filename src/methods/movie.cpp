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

    // const std::string info = tif.getMetadata();
    // bool mData = false;
    // if (info.find("ImageJ") != std::string::npos)
    // {
    //     std::string extra = tif.getIJMetadata();
    //     if (extra.size() > 1)
    //         mData = meta.parseImageJ(info + tif.getIJMetadata());
    // }
    // else if (info.find("OME") != std::string::npos)
    //     mData = meta.parseOME(info);

    // if (!mData)
    // {
    //     // if no fancy metada is presented, we go witht the basic one
    //     meta.SignificantBits = tif.getBitCount();
    //     meta.SizeT = tif.getNumDirectories();
    //     meta.SizeC = 1;
    //     meta.SizeZ = 1;
    //     meta.SizeY = tif.getHeight();
    //     meta.SizeX = tif.getWidth();

    //     meta.PhysicalSizeXY = 1.0;
    //     meta.PhysicalSizeZ = 1.0;
    //     meta.TimeIncrement = 1.0;

    //     meta.acquisitionDate = tif.getDateTime();
    //     meta.DimensionOrder = "XYCZT";
    //     meta.PhysicalSizeXYUnit = "Pixel";
    //     meta.PhysicalSizeZUnit = "Pixel";
    //     meta.TimeIncrementUnit = "Frame";

    //     meta.metaString = info;
    //     meta.nameCH.emplace_back("channe0");
    // }

    // // Check if axes are correct
    // if (meta.DimensionOrder.compare("XYCZT") != 0)
    // {
    //     mail->createMessage<MSG_Warning>("Movie has wrong axes format -> " + filename);
    //     return false;
    // }

    // if (meta.SizeZ > 1)
    // {
    //     mail->createMessage<MSG_Warning>("ERROR: SizeZ > 1 -> " + filename);
    //     return false;
    // }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////

    // vImg = new MatrixXd *[meta.SizeC];
    // for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    //     vImg[ch] = new MatrixXd[meta.SizeT];

    // uint32_t
    //     counter = 0,
    //     nImg = tif.getNumDirectories();

    // Message *ptr = mail->createMessage<MSG_Progress>("Loading images");
    // ptr->update(0.0f);

    // for (uint32_t t = 0; t < meta.SizeT; t++)
    //     for (uint32_t ch = 0; ch < meta.SizeC; ch++)
    //     {
    //         if (meta.SignificantBits == 8)
    //             vImg[ch][t] = tif.getImage<uint8_t>(counter).cast<double>();

    //         else if (meta.SignificantBits == 16)
    //             vImg[ch][t] = tif.getImage<uint16_t>(counter).cast<double>();

    //         else if (meta.SignificantBits == 32)
    //             vImg[ch][t] = tif.getImage<uint32_t>(counter).cast<double>();

    //         ptr->update(++counter / float(nImg));

    //     } // loop-images

    // ptr->markAsRead();

    // return true;

} // constructor

// Movie::~Movie(void)
// {
//     for (uint32_t ch = 0; ch < meta.SizeC; ch++)
//         delete[] vImg[ch];

//     delete[] vImg;
// } // destructor

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
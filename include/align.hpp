#pragma once
#include <config.hpp>

struct TransformData
{
    uint32_t width, height; // image dimensions
    double dx, dy;          // translation
    double cx, cy, angle;   // angle
    double sx, sy;          // scaling

    MatrixXd trf;  // Transform matrix
    MatrixXd itrf; // Inverse transform

    void updateTransform(void);

}; // TransformData -- struct

MatrixXd treatImage(MatrixXd img, int medianSize, double clipLimit,
                    uint32_t tileSizeX, uint32_t tileSizeY, Mailbox *mail);

class Align
{
private:
    // Mailbox *mail = nullptr;
    uint32_t numFrames;             // NUmber of frames
    Image<uint8_t> *img1 = nullptr; // Pointer to images of channel 0
    Image<uint8_t> *img2 = nullptr; // Pointer to images of channel 1

    uint32_t width, height; // Image dimensions
    TransformData RT;       // Holds all the optmized transformation paramters

    Mailbox *mail = nullptr;

    // Parallelization
    Threadpool pool;
    MatrixXd itrf; // inverse transform for alingment
    std::vector<double> global_energy;

public:
    Align(Mailbox *mailbox);
    ~Align(void) = default;

    void setImageData(const uint32_t numDir, Image<uint8_t> *im1, Image<uint8_t> *im2);

    bool alignCameras(void);
    bool correctAberrations(void);

    TransformData &getTransformData(void) { return RT; }
    const TransformData &getTransformData(void) const { return RT; }

    ////////////////// NOT-TO-USE /////////////////////
    void calcEnergy(const uint32_t id);
    double weightTransRot(const VectorXd &p);
    double weightScale(const VectorXd &p);

}; // class-Align

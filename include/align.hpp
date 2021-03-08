#pragma once
#include <config.hpp>

struct TransformData
{
    uint32_t width, height;
    double dx, dy;        // translation
    double cx, cy, angle; // angle
    double sx, sy;        // scaling

    MatrixXd trf;  // Transform matrix
    MatrixXd itrf; // Inverse transform

    void reset(uint32_t width, uint32_t height);
    void updateTransform(void);

}; // TransformData -- struct

class Align
{
private:
    Mailbox *mail = nullptr;

    uint32_t numFrames;                       // NUmber of frames
    uint32_t chAligning;                      // channel currently aligning
    std::vector<Image<uint8_t>> vImg0, vImg1; // Treated images

    uint32_t width, height;           // Image dimensions
    std::vector<TransformData> vecRT; // Holds all the optmized transformation paramters

    // Parallel variables
    MatrixXd itrf; // inverse transform for alingment
    std::vector<double> global_energy;

public:
    Align(Mailbox *mailbox, uint32_t nChannels = 2);
    ~Align(void) = default;

    void setChannelToAlign(uint32_t channel) { chAligning = channel - 1; }
    void setImageData(const uint32_t nFrames, MatrixXd *im1, MatrixXd *im2);

    bool alignCameras(void);
    bool correctAberrations(void);

    uint32_t getChannelIndex(void) const { return chAligning + 1; }

    TransformData &getTransformData(uint32_t channel) { return vecRT[channel - 1]; }
    const TransformData &getTransformData(uint32_t channel) const { return vecRT[channel - 1]; }

    ////////////////// NOT-TO-USE /////////////////////
    void calcEnergy(const uint32_t id);
    double weightTransRot(const VectorXd &p);
    double weightScale(const VectorXd &p);

}; // class-Align

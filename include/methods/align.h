#pragma once
#include "header.h"

struct TransformData
{
    TransformData(uint32_t width, uint32_t height);
    ~TransformData(void) = default;

    glm::ivec2 size;
    glm::vec2 translate, scale;
    glm::vec3 rotate;

    glm::mat3 trf, itrf; // Transform matrix

    void update(void);

}; // TransformData -- struct

// class Align
// {
// public:
//     Align(Movie *mov, Mailbox *mail);
//     ~Align(void) = default;

//     void setChannelToAlign(uint32_t channel) { chAligning = channel - 1; }
//     void setImageData(const uint32_t nFrames, MatrixXd *im1, MatrixXd *im2);

//     bool alignCameras(void);
//     bool correctAberrations(void);

//     uint32_t getChannelIndex(void) const { return chAligning + 1; }

//     TransformData &getTransformData(uint32_t channel) { return vecRT[channel - 1]; }
//     const TransformData &getTransformData(uint32_t channel) const { return vecRT[channel - 1]; }

//     ////////////////// NOT-TO-USE /////////////////////
//     void calcEnergy(const uint32_t id);
//     double weightTransRot(const VectorXd &p);
//     double weightScale(const VectorXd &p);

// private:
//     Mailbox *mail = nullptr;

//     uint32_t numFrames;                       // NUmber of frames
//     uint32_t chAligning;                      // channel currently aligning
//     std::vector<Image<uint8_t>> vImg0, vImg1; // Treated images

//     uint32_t width, height;           // Image dimensions
//     std::vector<TransformData> vecRT; // Holds all the optmized transformation paramters

//     // Parallel variables
//     MatrixXd itrf; // inverse transform for alingment
//     std::vector<double> global_energy;

// }; // class-Align

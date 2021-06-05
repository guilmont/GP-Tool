#pragma once
#include "header.h"

struct TransformData
{
    API TransformData(uint32_t width, uint32_t height);
    API TransformData(void) = default;
    API ~TransformData(void) = default;

    glm::ivec2 size;
    glm::vec2 translate, scale;
    glm::vec3 rotate;

    glm::mat3 trf;  // Transform matrix 0 to x
    glm::mat3 itrf; // Transform matrix x to 0

    API void update(void);

}; // TransformData -- struct

class Align
{
public:
    API Align(uint32_t nFrames, const MatXd *vim1, const MatXd *vim2, Mailbox *mail = nullptr);
    API ~Align(void) = default;

    API bool alignCameras(void);
    API bool correctAberrations(void);

    API const TransformData &getTransformData(void) const { return RT; }

private:
    Mailbox *mbox = nullptr;

    TransformData RT;                       // Transformation parameters
    std::vector<Image<uint8_t>> vIm0, vIm1; // Treated images

    // Parallel variables
    glm::mat3 itrf;
    std::vector<double> global_energy;

    void calcEnergy(const uint32_t id, const uint32_t nThr);
    double weightTransRot(const VecXd &p);
    double weightScale(const VecXd &p);

}; // class-Align

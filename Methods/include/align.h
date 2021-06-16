#pragma once
#include "header.h"
#include "goptimize.h"

struct TransformData
{
    GP_API TransformData(uint32_t width, uint32_t height);
    GP_API TransformData(void) = default;
    GP_API ~TransformData(void) = default;

    glm::ivec2 size;
    glm::vec2 translate, scale;
    glm::vec3 rotate;

    glm::mat3 trf;  // Transform matrix 0 to x
    glm::mat3 itrf; // Transform matrix x to 0

    GP_API void update(void);

};

class Align
{
public:
    GP_API Align(uint32_t nFrames, const MatXd *vim1, const MatXd *vim2);
    GP_API ~Align(void) = default;

    GP_API bool alignCameras(void);
    GP_API bool correctAberrations(void);

    GP_API const TransformData &getTransformData(void) const { return RT; }

private:
    TransformData RT;                       // Transformation parameters
    std::vector<Image<uint8_t>> vIm0, vIm1; // Treated images

    // Parallel variables
    glm::mat3 itrf;
    std::vector<double> global_energy;

    void calcEnergy(const uint32_t id, const uint32_t nThr);
    double weightTransRot(const VecXd &p);
    double weightScale(const VecXd &p);

}; // class-Align

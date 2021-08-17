#pragma once
#include "header.h"
#include "goptimize.h"

namespace GPT
{
    struct TransformData
    {
        GP_API TransformData(uint32_t width, uint32_t height);
        GP_API TransformData(void) = default;
        GP_API ~TransformData(void) = default;

        Vec2u size;
        Vec2d translate, scale;
        Vec3d rotate;

        Mat3d trf;  // Transform matrix 0 to x
        Mat3d itrf; // Transform matrix x to 0

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

        void stop(void)
        {
            if (nms)
                nms->stop();
        }

    private:
        TransformData RT;                       // Transformation parameters
        std::vector<Image<uint8_t>> vIm0, vIm1; // Treated images

        std::unique_ptr<GOptimize::NMSimplex> nms = nullptr;

        // Parallel variables
        Mat3d itrf;
        std::vector<double> global_energy;

        void calcEnergy(const uint32_t id, const uint32_t nThr);
        double weightTransRot(const VecXd &p);
        double weightScale(const VecXd &p);

    }; // class-Align

}
#pragma once

#include "header.h"
#include "goptimize.h"

namespace GPT
{
    // Input matrices must have the following column order:
    //  frame, time, pos_x, pos_y, error_x, error_y

    class GP_FBM
    {
    public:
        struct DA
        {
            double D, A;
            Vec2d mu;
        };

        struct CDA
        {
            std::vector<DA> da;
            double DR, AR;
        };

        struct ParticleID
        {
            uint64_t trackID, trajID;
        };

    public:
        GP_API GP_FBM(const MatXd &mat);                 // Constructor for single trajectory
        GP_API GP_FBM(const std::vector<MatXd> &vMat);   // Constructor for multiple trajectories
        GP_API ~GP_FBM(void);

        std::vector<ParticleID> partID;

        GP_API DA *singleModel(uint64_t id = 0);
        GP_API CDA *coupledModel(void);

        GP_API MatXd distrib_singleModel(uint64_t sample_size = 10000, uint64_t id = 0);
        GP_API MatXd distrib_coupledModel(uint64_t sample_size = 10000);
        
        GP_API MatXd calcAvgTrajectory(const VecXd &vTime, uint64_t id = 0);
        GP_API MatXd estimateSubstrateMovement();

        GP_API uint64_t getNumParticles(void) const { return nParticles; }
        GP_API void stop(void);


    private:
        void initialize(void);
        double weightSingle(const VecXd &DA);
        double weightCoupled(const VecXd &DA);

    private:
        uint64_t runID;
        double thresSimplex = 1e-4; // precision for simplex optimization
        const int64_t minSizePerTraj = 50;

        std::unique_ptr<GOptimize::NMSimplex> nms = nullptr;

        // As we will need this in many places, we shall keep a local copy
        std::vector<DA*> m_da;
        CDA *m_cda = nullptr;

        uint64_t nParticles;
        std::vector<MatXd> route; // Holds trajectories of all particles
        MatXd cRoute;             // concatenates all trajectories in one

    };

}
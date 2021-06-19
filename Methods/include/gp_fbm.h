#pragma once

#include "header.h"
#include "goptimize.h"


// Input matrices must have the following column order:
//  frame, time, pos_x, pos_y, error_x, error_y

class GP_FBM
{
public:
    struct DA
    {
        double D, A;
        glm::dvec2 mu;
    };

    struct CDA
    {
        std::vector<DA> da;
        double DR, AR;
    };

    struct ParticleID
    {
        uint32_t trackID, trajID;
    };

public:
    GP_API GP_FBM(const MatXd &mat);                 // Constructor for single trajectory
    GP_API GP_FBM(const std::vector<MatXd> &vMat);   // Constructor for multiple trajectories

    std::vector<ParticleID> partID;

    GP_API DA *singleModel(uint32_t id = 0);
    GP_API CDA *coupledModel(void);
    GP_API void stop(void);

    GP_API const MatXd &distrib_singleModel(uint32_t sample_size = 10000, uint32_t id = 0);
    GP_API const MatXd &distrib_coupledModel(uint32_t sample_size = 10000);
    
    GP_API const MatXd &calcAvgTrajectory(const VecXd &vTime, uint32_t id = 0, bool redo = false);
    GP_API const MatXd &estimateSubstrateMovement(bool redo = false);

    GP_API uint32_t getNumParticles(void) const { return nParticles; }


private:

    void initialize(void);
    double weightSingle(const VecXd &DA);
    double weightCoupled(const VecXd &DA);

    // This is not ideal, cuz it should be identical (a priori)
    // to the one in method trajectory
    enum Track : uint32_t
    {
        FRAME = 0,
        TIME = 1,
        POSX = 2,
        POSY = 3,
        ERRX = 4,
        ERRY = 5,
        NCOLS = 6
    };

private:
    uint32_t runID;
    double thresSimplex = 1e-4; // precision for simplex optimization
    const uint32_t minSizePerTraj = 50;

    std::unique_ptr<GOptimize::NMSimplex> nms = nullptr;

    std::vector<std::unique_ptr<DA>> v_da;
    std::unique_ptr<CDA> cpl_da = nullptr;

    uint32_t nParticles;
    std::vector<MatXd> route; // Holders all particles's routes
    MatXd cRoute;             // concatenates all routes in one

    // We save these results first time functions are called, after we just return these
    std::unique_ptr<MatXd> substrate = nullptr;
    std::vector<std::unique_ptr<MatXd>> average;

    std::unique_ptr<MatXd> distribCoupled = nullptr;
    std::vector<std::unique_ptr<MatXd>> distribSingle;

}; // class-GP_FBM

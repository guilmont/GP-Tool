#pragma once

#include "header.h"

#include <vector>

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

public:
    API GP_FBM(const MatXd &mat, Mailbox *mail = nullptr);
    API GP_FBM(const std::vector<MatXd> &vMat, Mailbox *mail = nullptr);

    API DA *singleModel(uint32_t id = 0);
    API CDA *coupledModel(void);

    // MatXd distrib_singleModel(uint32_t sample_size, uint32_t id = 0);
    // MatXd distrib_coupledModel(uint32_t sample_size);

    // MatXd calcAvgTrajectory(const VectorXd &vFrame, const VectorXd &vTime,
    //                         uint32_t id = 0);

    // MatXd estimateSubstrateMovement(void);

private:
    Mailbox *mbox = nullptr;

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

    std::vector<std::unique_ptr<DA>> v_da;
    std::unique_ptr<CDA> cpl_da = nullptr;

    uint32_t nParticles;
    std::vector<MatXd> route; // Holders all particles's routes
    MatXd cRoute;             // concatenates all routes in one

    // // We save these results first time functions are called, after we just return these
    // MatXd substrate;
    // MatXd distribCoupled;
    std::vector<MatXd> distribSingle;
    std::vector<MatXd> avgTraj;

}; // class-GP_FBM

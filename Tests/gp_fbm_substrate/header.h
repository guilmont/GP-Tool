#pragma once

#include <iostream>
#include "gp_fbm.h"

enum Track : uint32_t
{
    FRAME = 0,
    TIME = 1,
    POSX = 2,
    POSY = 3,
    ERRX = 4,
    ERRY = 5,
    NCOLS
};

enum Pmtr : uint32_t
{
    DT,
    PREC,
    OCC
};


static int getIndex(const VecXd &vec, double val)
{
    for (uint32_t k = 0; k < vec.size(); k++)
        if (vec[k] == val)
            return k;

    return -1;
} // getIndex

static void conditionalPush(std::vector<double> &vec, double value)
{
    for (double a : vec)
        if (value == a)
            return;

    vec.push_back(value);
}

static void addOcclusions(const double occlusion, MatXd& traj)
{
    std::random_device rd;
    std::default_random_engine ran(rd());
    std::uniform_real_distribution<double> unif(0.0, 1.0);

    const int nFrames = int(traj.rows()) - 1;
    const uint32_t nCols = uint32_t(traj.cols());

    for (int k = nFrames; k >= 0; k--)
        if (unif(ran) < occlusion)
        {
            uint32_t nRows = uint32_t(traj.rows()) - 1;

            traj.block(k, 0, nRows - k, nCols) = traj.block(k + 1, 0, nRows - k, nCols);
            traj.conservativeResize(nRows, nCols);
        }

} // addOcclusions

static MatXd GenTrajectories(uint32_t nFrames, double D, double A, double DT, double PREC)
{

    // Prepare normal random matrix
    std::random_device rd;
    std::default_random_engine ran(rd());
    std::uniform_real_distribution<double> unif(0.0, 1.0);
    std::normal_distribution<double> normal(0.0, 1.0);
    std::lognormal_distribution<double> lognormal(log(PREC + 1e-5), 0.05);

    auto ranN = [&]()
    { return normal(ran); };
    auto ranLN = [&]()
    { return lognormal(ran); };

    // Create kernel
    VecXd FR = VecXd::LinSpaced(nFrames, 0, nFrames - 1);

    VecXd T = DT * FR;
    VecXd TA = T.array().pow(A);

    MatXd kernel(nFrames, nFrames);
    for (uint32_t k = 0; k < nFrames; k++)
    {
        kernel(k, k) = 2.0f * D * TA(k);
        for (uint32_t l = k + 1; l < nFrames; l++)
        {
            kernel(k, l) = D * (TA(k) + TA(l) - pow(abs(T(l) - T(k)), A));
            kernel(l, k) = kernel(k, l);
        }
    }
    VecXd diag = kernel.diagonal();

    // Generating trajectories
    MatXd traj(nFrames, Track::NCOLS);
    traj.col(Track::FRAME) = std::move(FR);
    traj.col(Track::TIME) = std::move(T);

    for (uint32_t k = 0; k < 2; k++)
    {
        traj.col(Track::ERRX + k) = VecXd::NullaryExpr(nFrames, ranLN);

        kernel.diagonal() = diag.array() + traj.col(Track::ERRX + k).array().square();
        MatXd L = kernel.llt().matrixL();

        traj.col(Track::POSX + k) = L * VecXd::NullaryExpr(nFrames, ranN);
    }


    return traj;

} // GenTrajectories
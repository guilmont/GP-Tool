#include "methods/gp_fbm.h"

#include <random>

#include "eigen/Eigen/LU"
#include "eigen/Eigen/Cholesky"

static void removeRow(MatXd &mat, uint32_t row)
{
    uint32_t nRows = uint32_t(mat.rows()) - 1;
    uint32_t nCols = uint32_t(mat.cols());

    if (row < nRows)
        mat.block(row, 0, nRows - row, nCols) = mat.block(row + 1, 0, nRows - row, nCols);

    mat.conservativeResize(nRows, nCols);
} // removeRow

///////////////////////////////////////////////////////////////////////////////
// HELPER FUNCTIONS

static MatXd calcKernel(const double D, const double A,
                        const VecXd &T1, const VecXd &T2)
{
    const uint32_t
        NROWS = uint32_t(T1.size()),
        NCOLS = uint32_t(T2.size());

    const VecXd
        TA1 = T1.array().pow(A),
        TA2 = T2.array().pow(A);

    MatXd kernel(NROWS, NCOLS);

    if (NROWS == NCOLS)
    {
        for (uint32_t k = 0; k < NROWS; k++)
            for (uint32_t l = k; l < NCOLS; l++)
            {
                kernel(k, l) = D * (TA1(k) + TA2(l) - pow(abs(T2(l) - T1(k)), A));
                kernel(l, k) = kernel(k, l);
            }
    } // if-square-matrix

    else
    {
        for (uint32_t k = 0; k < NROWS; k++)
            for (uint32_t l = 0; l < NCOLS; l++)
                kernel(k, l) = D * (TA1(k) + TA2(l) - pow(abs(T2(l) - T1(k)), A));
    }

    return kernel;

} // calcKernel

// static MatXd calcDensity(const MatXd &mcmc)
// {
//     const uint32_t nHisto = mcmc.cols(),
//                    nRows = mcmc.rows();

//     // Using Sturges' rule for histogram bin
//     const uint32_t nPoints = 1 + ceil(log2(double(nRows))),
//                    nCols = 2 * nHisto;

//     MatXd mHisto(nPoints, nCols);
//     mHisto.fill(0);

//     for (uint32_t histo = 0; histo < nHisto; histo++)
//     {
//         // Calculate min and max values for bin width
//         double minValue = INFINITY, maxValue = -INFINITY;
//         for (uint32_t k = 0; k < nRows; k++)
//         {
//             minValue = mcmc(k, histo) < minValue ? mcmc(k, histo) : minValue;
//             maxValue = mcmc(k, histo) > maxValue ? mcmc(k, histo) : maxValue;
//         }

//         double binWidth = (maxValue - minValue) / nPoints;

//         mHisto.col(2 * histo) = VecXd::LinSpaced(nPoints, minValue, maxValue);

//         for (uint32_t k = 0; k < nRows; k++)
//         {
//             uint32_t id = std::min<uint32_t>((mcmc(k, histo) - minValue) / binWidth, nPoints - 1);
//             mHisto(id, 2 * histo + 1)++;
//         }

//         double sum = mHisto.col(2 * histo + 1).sum();
//         mHisto.col(2 * histo + 1).array() /= sum;

//     } // loop-cols

//     return mHisto;

// } // calcDensity

///////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS

GP_FBM::GP_FBM(const MatXd &mat, Mailbox *mail) : mbox(mail)
{
    nParticles = 1;
    route.push_back(mat);

    initialize();
} // constructor

GP_FBM::GP_FBM(const std::vector<MatXd> &vMat, Mailbox *mail) : mbox(mail)
{
    nParticles = uint32_t(vMat.size());
    for (const MatXd &mat : vMat)
        route.push_back(mat);

    initialize();
} // constructor

GP_FBM::DA *GP_FBM::singleModel(uint32_t id)
{
    // It is already calculated, so just return it
    if (v_da[id])
        return v_da[id].get();

    // Otherwise, calculate accordingly
    runID = id;
    VecXd vec(4);
    vec << log(0.5), log(0.5),
        route[id](0, Track::POSX), route[id](0, Track::POSY);

    GOptimize::NMSimplex nms(vec, thresSimplex);
    if (!nms.runSimplex(&GP_FBM::weightSingle, this))
    {
        std::string txt = "(GP_FBM::singleModel) Model didn't converge!";
        if (mbox)
            mbox->create<Message::Warn>(txt);
        else
            std::cout << "Warn " << txt << std::endl;

        return nullptr;
    }

    vec = nms.getResults();
    double eA = exp(vec(1));

    v_da[id] = std::make_unique<DA>();
    v_da[id]->D = exp(vec(0));
    v_da[id]->A = 2.0 * eA / (eA + 1.0);
    v_da[id]->mu = {vec[2], vec[3]};

    return v_da[id].get();

} // single model

GP_FBM::CDA *GP_FBM::coupledModel(void)
{

    if (nParticles < 2)
    {
        std::string txt = "(GP_FBM::coupledModel) Cannot run on single trajectory!!";

        if (mbox)
            mbox->create<Message::Error>(txt);
        else
            std::cerr << "ERROR " << txt << std::endl;

        return nullptr;
    }

    // If it is already calculated, just return it
    if (cpl_da)
        return cpl_da.get();

    // In order to remove substrate movement, we will improve in accuracy if we remove
    // frames that are not common to all trajectories. Because of that, let's make a hard
    // copy of original trajectories

    std::vector<MatXd> lRoute(route);

    // get maximum number of frames
    uint32_t maxFrames = 0;
    for (MatXd &mat : lRoute)
    {
        uint32_t lRow = uint32_t(mat.rows()) - 1;
        uint32_t fr = uint32_t(mat(lRow, Track::FRAME)) + 1; // Starts at zero, so +1

        maxFrames = std::max(maxFrames, fr);
    }

    // check if frame is in all movies
    std::vector<uint32_t> vCount(maxFrames, 0);
    for (MatXd &mat : lRoute)
        for (uint32_t k = 0; k < uint32_t(mat.rows()); k++)
        {
            uint32_t fr = uint32_t(mat(k, Track::FRAME));
            vCount[fr]++;
        }

    // Removing frames that are not present in all particles
    for (MatXd &mat : lRoute)
    {
        for (int k = uint32_t(mat.rows()) - 1; k >= 0; k--)
        {
            uint32_t fr = uint32_t(mat(k, Track::FRAME));
            if (vCount[fr] != nParticles)
                removeRow(mat, k);
        }

        if (uint32_t(mat.rows()) < minSizePerTraj)
        {
            std::string txt = "(GP_FBM::coupledModel) Not all trajectories intersect!!";
            if (mbox)
                mbox->create<Message::Warn>(txt);
            else
                std::cout << "WARN " << txt << std::endl;

            return nullptr;
        }

    } // loop-trajectories

    // Generate concatenated matrix
    uint32_t ct = 0;
    for (MatXd &mat : lRoute)
        ct += uint32_t(mat.rows());

    const uint32_t
        nCols = uint32_t(lRoute[0].cols()),
        nRows = uint32_t(lRoute[0].rows());

    cRoute = MatXd(ct, nCols);

    ct = 0;
    VecXd vec(2 * (nParticles + 1)); // Last two for substrate

    vec(2 * nParticles) = 0.0;     // DR
    vec(2 * nParticles + 1) = 1.0; // AR

    for (uint32_t k = 0; k < nParticles; k++)
    {
        DA *da = singleModel(k);
        vec(2 * k) = log(da->D);
        vec(2 * k + 1) = -log(2.0 / da->A - 1.0);

        lRoute[k].col(Track::POSX).array() -= da->mu.x;
        lRoute[k].col(Track::POSY).array() -= da->mu.y;

        cRoute.block(ct, 0, nRows, nCols) = lRoute[k];
        ct += nRows;
    }

    // Optimizing dynamics parameters
    GOptimize::NMSimplex nms(vec, thresSimplex);
    if (!nms.runSimplex(&GP_FBM::weightCoupled, this))
    {
        std::string txt = "(GP_FBM::coupledModel) Model didn't converge!";
        if (mbox)
            mbox->create<Message::Warn>(txt);
        else
            std::cout << "Warn " << txt << std::endl;

        return nullptr;
    }

    // Results were successful, let's create the final pointer
    vec = nms.getResults().array().exp();

    cpl_da = std::make_unique<CDA>();

    // Substrate
    double *R = vec.data() + 2 * nParticles;
    cpl_da->DR = R[0];
    cpl_da->AR = 2.0f * R[1] / (1.0f + R[1]);

    // Individual particles
    cpl_da->da.resize(nParticles);
    for (uint32_t k = 0; k < nParticles; k++)
    {
        double *lda = vec.data() + 2 * k;
        cpl_da->da[k].D = lda[0];
        cpl_da->da[k].A = 2.0 * lda[1] / (1.0 + lda[1]);
    }

    return cpl_da.get();
} // coupledModel

// ////////////// ESTIMATE TRAJECTORIES  /////////////////////

// void GP_FBM::calcAvgTrajectory(const VecXd &vFrame, const VecXd &vTime,
//                                const uint32_t id)
// {
//     if (id >= route.size())
//         mail->createMessage<MSG_Error>("(GP_FBM::calcAvgTrajectory) id doesn't exist!!");

//     // Create output matrix
//     MatXd traj(vTime.size(), route[id].cols());
//     traj.col(TrajColumns::FRAME) = vFrame;
//     traj.col(TrajColumns::TIME) = vTime;

//     // Getting particle's dynamical parameters
//     const double
//         D = parSingle[id].D,
//         A = parSingle[id].A;

//     // Importing trajectory
//     MatXd mat = route[id];
//     for (uint32_t k = 0; k < nDims; k++)
//         mat.col(TrajColumns::POSX + 2 * k).array() -= parSingle[id].mu[k];

//     const VecXd &VT = mat.col(TrajColumns::TIME);

//     MatXd
//         K = calcKernel(D, A, VT, VT),
//         Ks = calcKernel(D, A, VT, vTime),
//         KsT = Ks.transpose(),
//         K2s = calcKernel(D, A, vTime, vTime);

//     VecXd diagK = K.diagonal();

//     for (uint8_t ch = 0; ch < nDims; ch++)
//     {
//         K.diagonal() = diagK + route[id].col(TrajColumns::ERRX + 2 * ch);

//         // Calculating particle's expected position
//         Eigen::LLT<MatXd> cholesky = K.llt();
//         VecXd alpha = cholesky.solve(mat.col(TrajColumns::POSX + 2 * ch));

//         traj.col(TrajColumns::POSX + 2 * ch) = KsT * alpha;
//         traj.col(TrajColumns::POSX + 2 * ch).array() += parSingle[id].mu[ch];

//         // Calculating positioning expected error

//         const VecXd ver1 = K2s.diagonal();
//         const VecXd ver2 = (KsT * cholesky.solve(Ks)).diagonal();

//         traj.col(TrajColumns::ERRX + 2 * ch) = (ver1 - ver2).array().sqrt();
//         traj(0, TrajColumns::ERRX + 2 * ch) = sqrt(route[id](0, TrajColumns::ERRX + 2 * ch));
//     }

//     vecAvgTraj[id] = traj;

// } // dataTreatment

// void GP_FBM::estimateSubstrateMovement(void)
// {
//     if (!bControl[COUPLED])
//     {
//         mail->createMessage<MSG_Warning>("(GP_FBM::estimateSubstrateMovement) "
//                                          "'coupledModel' must run before "
//                                          "estimateSubstrateMovement'!!");
//         if (!coupledModel())
//         {
//             mail->createMessage<MSG_Error>("(GP_FBM::estimateSubstrateMovement) "
//                                            "'coupledModel' didn't converge!");
//             return;
//         }
//     }
//     // Let's determine time points which we need to calculate substrate movement
//     // Determine maximum number of frames
//     uint32_t maxFR = 0;
//     for (auto &mat : route)
//     {
//         const uint32_t k = mat.rows() - 1;
//         maxFR = std::max<uint32_t>(maxFR, mat(k, TrajColumns::FRAME));
//     }

//     // Check all frames present
//     std::vector<std::pair<bool, double>> vfr(maxFR + 1);
//     for (auto &mat : route)
//     {
//         const uint32_t N = mat.rows();
//         for (uint32_t k = 0; k < N; k++)
//             vfr[int(mat(k, 0))] = {true, mat(k, TrajColumns::TIME)};
//     }

//     // We use those present
//     std::vector<double> auxFrame, auxTime;
//     for (uint32_t k = 0; k < vfr.size(); k++)
//         if (vfr[k].first)
//         {
//             auxFrame.push_back(k);
//             auxTime.push_back(vfr[k].second);
//         }

//     // Let's calculate average trajectories with precision
//     VecXd vFrame = VecXd::Map(auxFrame.data(), auxFrame.size());
//     VecXd vTime = VecXd::Map(auxTime.data(), auxTime.size());

//     std::vector<MatXd> avgRoute;
//     for (uint32_t k = 0; k < route.size(); k++)
//     {
//         calcAvgTrajectory(vFrame, vTime, k);
//         MatXd mat = getAvgTrajectory(k);
//         for (uint8_t ch = 0; ch < nDims; ch++)
//         {
//             // We will need variance for next steps
//             mat.col(TrajColumns::ERRX + 2 * ch).array() *=
//                 mat.col(TrajColumns::ERRX + 2 * ch).array();

//             // We should subtract average position to estimate substrate movement
//             mat.col(TrajColumns::POSX + 2 * ch).array() -= parSingle[k].mu[ch];
//         }

//         avgRoute.emplace_back(std::move(mat));
//     }

//     // We shall calculate all kernels now
//     std::vector<MatXd> vKernel(route.size());
//     for (uint32_t k = 0; k < route.size(); k++)
//     {
//         const double
//             D = parCoupled.particle[k].D,
//             A = parCoupled.particle[k].A;

//         vKernel[k] = calcKernel(D, A, vTime, vTime);
//     }

//     // Kernel for substrate
//     MatXd iKR = calcKernel(parCoupled.DR, parCoupled.AR, vTime, vTime);
//     iKR.diagonal().array() += 0.0001;
//     iKR = iKR.inverse();

//     // Let's estimate substrate movement
//     substrate = MatXd(vTime.size(), 2 * nDims + TrajColumns::POSX);
//     substrate.col(TrajColumns::FRAME) = vFrame;
//     substrate.col(TrajColumns::TIME) = vTime;

//     for (uint8_t ch = 0; ch < nDims; ch++)
//     {
//         MatXd A = iKR;
//         VecXd vec = VecXd::Zero(vTime.size());

//         for (uint32_t k = 0; k < route.size(); k++)
//         {
//             MatXd iK = vKernel[k];
//             iK.diagonal() += avgRoute[k].col(TrajColumns::ERRX + 2 * ch);
//             iK = iK.inverse();

//             A += iK;
//             vec += iK * avgRoute[k].col(TrajColumns::POSX + 2 * ch);

//         } // loop-particles

//         A = A.inverse();
//         substrate.col(TrajColumns::POSX + 2 * ch) = A * vec;
//         substrate.col(TrajColumns::ERRX + 2 * ch) = A.diagonal().array().sqrt();
//     } // loop-dims

//     bControl.set(SUBSTRATE);

// } // estimateSubstrateMovement

// /////////////////  RUN DISTRIBUTIONS ///////////////////
// void GP_FBM::distrib_singleModel(void)
// {
//     if (!bControl[SINGLE])
//     {
//         mail->createMessage<MSG_Warning>("(GP_FBM::distrib_singleModel) We need to "
//                                          "run optimizer before distribution!!");

//         if (!singleModel())
//         {
//             mail->createMessage<MSG_Error>("(GP_FBM::distrib_singleModel) 'singleModel' "
//                                            "didn't converge!");
//             return;
//         }
//     }

//     // Loop over all the trajectories
//     for (uint32_t id = 0; id < route.size(); id++)
//     {
//         runId = id;
//         ParamDA &p = parSingle.at(id);

//         uint32_t svec = 3;
//         svec = nDims == 2 ? 4 : svec;
//         svec = nDims == 3 ? 5 : svec;

//         VecXd vec(svec);
//         vec(0) = log(p.D);
//         vec(1) = -log(2.0 / p.A - 1.0);

//         for (uint8_t ch = 0; ch < nDims; ch++)
//             vec(ch + 2) = p.mu[ch];

//         MatXd mcmc =
//             GOptimize::sampleParameters<double, VecXd, MatXd>(
//                 vec, maxMCS, &transferSingle, this);

//         for (uint32_t k = 0; k < maxMCS; k++)
//         {
//             mcmc(k, 0) = exp(mcmc(k, 0)); // D

//             double eA = exp(mcmc(k, 1)); // A
//             mcmc(k, 1) = 2.0f * eA / (eA + 1.0f);
//         }

//         // Appending to class variable
//         distribSingle.push_back(calcDensity(mcmc));
//     } // loop-particles

//     // Signaling distribution was calculated
//     bControl.set(DTB_SINGLE);

// } // distrib_singleModel

// void GP_FBM::distrib_coupledModel(void)
// {
//     if (!bControl[COUPLED])
//     {
//         mail->createMessage<MSG_Warning>("(GP_FBM::distrib_coupledModel) Must run "
//                                          "coupled optimization first!!");

//         if (!coupledModel())
//         {
//             mail->createMessage<MSG_Error>("(GP_FBM::distrib_coupledModel) "
//                                            "'coupledModel' didn't converge!!");
//             return;
//         }
//     }

//     // Generating vector with values that maximize likelihood
//     const uint32_t nParticles = route.size();
//     VecXd vec(2 * nParticles + 2);

//     vec(2 * nParticles) = log(parCoupled.DR);
//     vec(2 * nParticles + 1) = -log(2.0 / parCoupled.AR - 1.0);

//     for (uint32_t k = 0; k < nParticles; k++)
//     {
//         ParamDA &p = parCoupled.particle.at(k);
//         vec(k) = log(p.D);
//         vec(nParticles + k) = -log(2.0 / p.A - 1.0);
//     }

//     // Generate distribution
//     MatXd mcmc = GOptimize::sampleParameters<double, VecXd, MatXd>(
//         vec, maxMCS, &transferCoupled, this);

//     // Let's convert results to approppriate space
//     for (uint32_t k = 0; k < maxMCS; k++)
//     {
//         for (uint32_t l = 0; l < nParticles; l++)
//         {
//             mcmc(k, 2 * l) = expf(mcmc(k, 2 * l)); // D

//             double val = expf(mcmc(k, 2 * l + 1));
//             mcmc(k, 2 * l + 1) = 2.0f * val / (val + 1.0f); // alpha
//         }

//         // substrate
//         const uint32_t num = 2 * nParticles;
//         mcmc(k, num) = expf(mcmc(k, num)); // D

//         double val = expf(mcmc(k, num + 1));
//         mcmc(k, num + 1) = 2.0f * val / (1.0f + val); // alpha

//     } // loop-rows

//     distribCoupled = calcDensity(mcmc);

//     // Signaling distribution was done
//     bControl.set(DTB_COUPLED);

// } // distrib_coupledModel

///////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

void GP_FBM::initialize(void)
{
    avgTraj.resize(nParticles);
    distribSingle.resize(nParticles);
    v_da.resize(nParticles);

    // Calculating the smallest timepoint accross all trajectories
    double minTime = INFINITY;
    for (MatXd &mat : route)
        minTime = std::min(mat(0, Track::TIME), minTime);

    for (MatXd &mat : route)
    {
        const uint32_t nRows = uint32_t(mat.rows());
        for (uint32_t k = 0; k < nRows; k++)
        {
            mat(k, Track::TIME) -= minTime;
            mat(k, Track::ERRX) *= mat(k, Track::ERRX);
            mat(k, Track::ERRY) *= mat(k, Track::ERRY);
        }

    } // loop-routes

} // function

double GP_FBM::weightSingle(const VecXd &DA)
{
    // Rescaling parameters to proper values
    double
        D = exp(DA(0)),
        eA = exp(DA(1)),
        A = 2.0 * eA / (eA + 1.0);

    // Building FBM kernel
    const VecXd &vT = route[runID].col(Track::TIME);
    MatXd kernel = calcKernel(D, A, vT, vT);
    kernel.diagonal().array() += 1e-4;

    VecXd diag = kernel.diagonal();

    // LIKELIHOOD ::  The weight function is given by the negative of likelihood
    double weight = 0;
    for (uint8_t k = 0; k < 2; k++)
    {
        VecXd vec = route[runID].col(Track::POSX + k).array() - DA(2 + k);

        kernel.diagonal() = diag + route[runID].col(Track::ERRX + k);
        Eigen::LLT<MatXd> cholesky = kernel.llt();
        VecXd alpha = cholesky.solve(vec);
        MatXd L = cholesky.matrixL();
        weight += 0.5 * vec.dot(alpha) + L.diagonal().array().log().sum();
    }

    // flat prior on A
    return weight - DA(1) + 2.0 * log(1.0 + eA) - DA(0);

} //weightSingle

double GP_FBM::weightCoupled(const VecXd &DA)
{
    const uint32_t N = uint32_t(cRoute.rows());

    VecXd eDA = DA.array().exp(),
          D(nParticles), A(nParticles);

    for (uint32_t k = 0; k < nParticles; k++)
    {
        D(k) = eDA[2 * k];
        A(k) = 2.0f * eDA[2 * k + 1] / (1.0f + eDA[2 * k + 1]);
    }

    double DR = eDA[2 * nParticles],
           AR = 2.0f * eDA[2 * nParticles + 1] / (eDA[2 * nParticles + 1] + 1.0);

    // Create complete kernel
    MatXd kernel(N, N);

    const uint32_t nRows = uint32_t(cRoute.rows() / nParticles);
    const VecXd &vt = cRoute.block(0, Track::TIME, nRows, 1);

    uint32_t sizeCol = 0, sizeRow = 0;
    for (uint32_t k = 0; k < nParticles; k++)
    {

        MatXd loc = calcKernel(D[k], A[k], vt, vt);
        MatXd RR = calcKernel(DR, AR, vt, vt);

        kernel.block(sizeRow, sizeCol, nRows, nRows) = loc + RR;

        sizeCol += nRows;
        for (uint32_t l = k + 1; l < nParticles; l++)
        {
            kernel.block(sizeRow, sizeCol, nRows, nRows) = RR;
            kernel.block(sizeCol, sizeRow, nRows, nRows) = RR.transpose();
            sizeCol += nRows;
        }

        sizeRow += nRows;
        sizeCol = sizeRow;
    } // loop-particles

    /////////////////////////////////////////////////////////////////
    // LIKELIHOOD
    // The weight function is given by the negative of likelihood

    double weight = 0;
    VecXd diag = kernel.diagonal();

    for (uint8_t k = 0; k < 2; k++)
    {
        const VecXd &vec = cRoute.col(Track::POSX + k);

        // X direction
        kernel.diagonal() = diag + cRoute.col(Track::ERRX + k);
        Eigen::LLT<MatXd> cholesky = kernel.llt();
        VecXd alpha = cholesky.solve(vec);
        MatXd L = cholesky.matrixL();
        weight += 0.5 * vec.dot(alpha);
        weight += L.diagonal().array().log().sum();
    }

    // Flat priors for all parameters
    for (uint32_t k = 0; k <= nParticles; k++)
    {
        weight -= DA(2 * k);
        weight -= (DA(2 * k + 1) - 2.0 * log(1.0 + eDA(2 * k + 1)));
    }

    return weight;

} // weightCoupled
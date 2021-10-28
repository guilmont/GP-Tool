#include "gp_fbm.h"


namespace GPT
{
    ///////////////////////////////////////////////////////////////////////////////
    // HELPER FUNCTIONS

    static void removeRow(MatXd &mat, uint64_t row)
    {
        uint64_t 
            nRows = mat.rows() - 1,
            nCols = mat.cols();

        if (row < nRows)
            mat.block(row, 0, nRows - row, nCols) = mat.block(row + 1, 0, nRows - row, nCols);

        mat.conservativeResize(nRows, nCols);
    }

    static MatXd calcKernel(const double D, const double A, const VecXd &T1, const VecXd &T2)
    {
        const uint64_t
            NROWS = T1.size(),
            NCOLS = T2.size();

        const VecXd
            TA1 = T1.array().pow(A),
            TA2 = T2.array().pow(A);

        MatXd kernel(NROWS, NCOLS);

        if (NROWS == NCOLS)
        {
            for (uint64_t k = 0; k < NROWS; k++)
                for (uint64_t l = k; l < NCOLS; l++)
                {
                    kernel(k, l) = D * (TA1(k) + TA2(l) - pow(abs(T2(l) - T1(k)), A));
                    kernel(l, k) = kernel(k, l);
                }
        } // if-square-matrix

        else
        {
            for (uint64_t k = 0; k < NROWS; k++)
                for (uint64_t l = 0; l < NCOLS; l++)
                    kernel(k, l) = D * (TA1(k) + TA2(l) - pow(abs(T2(l) - T1(k)), A));
        }

        return kernel;

    } // calcKernel

    ///////////////////////////////////////////////////////////////////////////////
    // PUBLIC FUNCTIONS

    GP_FBM::GP_FBM(const MatXd &mat)
    {
        nParticles = 1;
        route.push_back(mat);
        initialize();
    }

    GP_FBM::GP_FBM(const std::vector<MatXd> &vMat)
    {
        nParticles = vMat.size();
        route.resize(nParticles);

        std::copy(vMat.begin(), vMat.end(), route.begin());
        initialize();
    }

    GP_FBM::~GP_FBM(void)
    {
        for (DA* da : m_da)
            if (da)
                delete da;

        if (m_cda)
            delete m_cda;
    }

 
    GP_FBM::DA* GP_FBM::singleModel(uint64_t id)
    {

        if (m_da[id])
            return m_da[id];

        // Otherwise, calculate accordingly
        runID = id;
        VecXd vec(4);
        vec << log(0.5), log(0.5),
            route[id](0, Track::POSX), route[id](0, Track::POSY);

        nms = std::make_unique<GOptimize::NMSimplex>(vec, thresSimplex);
        if (!nms->runSimplex(&GP_FBM::weightSingle, this))
        {
            pout("WARN: (GP_FBM::singleModel) Model didn't converge!");
            return nullptr;
        }

        vec = nms->getResults();
        nms.release();

        double eA = exp(vec(1));
        m_da[id] = new DA();
        m_da[id]->D = exp(vec(0));
        m_da[id]->A = 2.0 * eA / (eA + 1.0);
        m_da[id]->mu = {vec[2], vec[3]};

        return m_da[id];
    }

    GP_FBM::CDA* GP_FBM::coupledModel(void)
    {
        if (m_cda)
            return m_cda;

        if (nParticles < 2)
        {
            pout("ERROR: (GP_FBM::coupledModel) Cannot run on single trajectory!!");
            exit(-1);
        }


        // In order to remove substrate movement, we will improve in accuracy if we remove
        // frames that are not common to all trajectories. Because of that, let's make a hard
        // copy of original trajectories

        std::vector<MatXd> lRoute(route);

        // get maximum number of frames
        uint64_t maxFrames = 0;
        for (MatXd &mat : lRoute)
        {
            uint64_t lRow = mat.rows() - 1;
            uint64_t fr = static_cast<uint64_t>(mat(lRow, Track::FRAME)) + 1; // Starts at zero, so +1

            maxFrames = std::max(maxFrames, fr);
        }

        // check if frame is in all movies
        std::vector<uint64_t> vCount(maxFrames, 0);
        for (MatXd &mat : lRoute)
            for (uint64_t k = 0; k < uint64_t(mat.rows()); k++)
                vCount[static_cast<uint64_t>(mat(k, Track::FRAME))]++;

        // Removing frames that are not present in all particles
        for (MatXd &mat : lRoute)
        {
            for (int64_t k = mat.rows() - 1; k >= 0; k--)
            {
                uint64_t fr = static_cast<uint64_t>(mat(k, Track::FRAME));
                if (vCount[fr] != nParticles)
                    removeRow(mat, k);
            }

            if (mat.rows() < minSizePerTraj)
            {
                pout("WARN: (GP_FBM::coupledModel) Not all trajectories intersect!!");
                return nullptr;
            }

        } // loop-trajectories

        // Generate concatenated matrix
        uint64_t ct = 0;
        for (MatXd &mat : lRoute)
            ct += mat.rows();

        const uint64_t
            nCols = lRoute[0].cols(),
            nRows = lRoute[0].rows();

        cRoute = MatXd(ct, nCols);

        ct = 0;
        VecXd vec(2 * (nParticles + 1)); // Last two for substrate

        vec(2 * nParticles) = 0.0;     // DR
        vec(2 * nParticles + 1) = 1.0; // AR

        for (uint64_t k = 0; k < nParticles; k++)
        {
            DA *da = singleModel(k);
            vec(2 * k) = log(da->D);
            vec(2 * k + 1) = -log(2.0 / da->A - 1.0);

            lRoute[k].col(Track::POSX).array() -= da->mu(0);
            lRoute[k].col(Track::POSY).array() -= da->mu(1);

            cRoute.block(ct, 0, nRows, nCols) = lRoute[k];
            ct += nRows;
        }

        // Optimizing dynamics parameters
        nms = std::make_unique<GOptimize::NMSimplex>(vec, thresSimplex);
        if (!nms->runSimplex(&GP_FBM::weightCoupled, this))
        {
            pout("WARN: (GP_FBM::coupledModel) Model didn't converge!");
            return nullptr;
        }

        // Results were successful, let's create the final pointer
        vec = nms->getResults().array().exp();
        nms.release();

        m_cda = new CDA();

        // Substrate
        double *R = vec.data() + 2 * nParticles;
        m_cda->DR = R[0];
        m_cda->AR = 2.0f * R[1] / (1.0f + R[1]);

        // Individual particles
        m_cda->da.resize(nParticles);
        for (uint64_t k = 0; k < nParticles; k++)
        {
            double* lda = vec.data() + 2 * k;
            m_cda->da[k].D = lda[0];
            m_cda->da[k].A = 2.0 * lda[1] / (1.0 + lda[1]);
        }

        return m_cda;
    } // coupledModel

    GP_API void GP_FBM::stop(void)
    {
        if (nms)
            nms->stop();
    }

    ///////////////////////////////////////////////////////////////////////////////
    // AVERAGE TRAJECTORIES ///////////////////////////////////////////////////////

    MatXd GP_FBM::calcAvgTrajectory(const VecXd &vTime, uint64_t id)
    {
        const uint64_t
            nRows = vTime.size(),
            nCols = route[id].cols();

        MatXd traj(nRows, nCols);         // Output matrix
        traj.col(Track::TIME) = vTime;

        // Getting particle's dynamical parameters
        DA *da = singleModel(id);

        // Importing trajectory
        MatXd mat = route[id];
        mat.col(Track::POSX).array() -= da->mu(0);
        mat.col(Track::POSY).array() -= da->mu(1);

        const VecXd &VT = mat.col(Track::TIME);

        MatXd K = calcKernel(da->D, da->A, VT, VT),
            Ks = calcKernel(da->D, da->A, VT, vTime),
            KsT = Ks.transpose(),
            K2s = calcKernel(da->D, da->A, vTime, vTime);

        VecXd diagK = K.diagonal();

        for (uint64_t k = 0; k < 2; k++)
        {
            K.diagonal() = diagK + route[id].col(Track::ERRX + k);

            // Calculating particle's expected position
            Eigen::LLT<MatXd> cholesky = K.llt();
            VecXd alpha = cholesky.solve(mat.col(Track::POSX + k));

            traj.col(Track::POSX + k) = KsT * alpha;
            traj.col(Track::POSX + k).array() += da->mu[k];

            // Calculating positioning expected error
            const VecXd ver1 = K2s.diagonal();
            const VecXd ver2 = (KsT * cholesky.solve(Ks)).diagonal();

            traj.col(Track::ERRX + k) = (ver1 - ver2).array().sqrt();
            traj(0, Track::ERRX + k) = sqrt(route[id](0, Track::ERRX + k));
        }

        return traj;
    }

    MatXd GP_FBM::estimateSubstrateMovement(void)
    {

        // Let's determine time points which we need to calculate substrate movement
        // Determine maximum number of frames
        uint64_t maxFR = 0;
        for (auto &mat : route)
        {
            const uint64_t k = mat.rows() - 1;
            maxFR = std::max(maxFR, static_cast<uint64_t>(mat(k, Track::FRAME)));
        }

        // Check all frames present
        std::vector<std::pair<bool, double>> vfr(maxFR + 1);
        for (auto &mat : route)
        {
            const uint64_t N = uint64_t(mat.rows());
            for (uint64_t k = 0; k < N; k++)
                vfr[int(mat(k, 0))] = {true, mat(k, Track::TIME)};
        }

        // We use those present
        std::vector<double> auxFrame, auxTime;
        for (uint64_t k = 0; k < vfr.size(); k++)
            if (vfr[k].first)
            {
                auxFrame.push_back(double(k));
                auxTime.push_back(vfr[k].second);
            }

        // Let's calculate average trajectories with precision
        VecXd vFrame = VecXd::Map(auxFrame.data(), auxFrame.size());
        VecXd vTime = VecXd::Map(auxTime.data(), auxTime.size());

        std::vector<MatXd> avgRoute;
        for (uint64_t k = 0; k < route.size(); k++)
        {
            MatXd mat = calcAvgTrajectory(vTime, k);
            for (uint8_t k = 0; k < 2; k++)
            {
                // We will need variance for next steps
                mat.col(Track::ERRX + k) *= mat.col(Track::ERRX + k);

                // We should subtract average position to estimate substrate movement
                DA *da = singleModel(k);
                mat.col(Track::POSX + k).array() -= da->mu[k];
            }

            avgRoute.emplace_back(std::move(mat));
        }

        // We shall calculate all kernels now
        CDA *cda = coupledModel();

        std::vector<MatXd> vKernel(route.size());
        for (uint64_t k = 0; k < route.size(); k++)
            vKernel[k] = calcKernel(cda->da[k].D, cda->da[k].A, vTime, vTime);

        // Kernel for substrate
        MatXd iKR = calcKernel(cda->DR, cda->AR, vTime, vTime);
        iKR.diagonal().array() += 0.0001;
        iKR = iKR.inverse();

        // Let's estimate substrate movement
        const uint64_t 
            nRows = static_cast<uint64_t>(vTime.size()),
            nCols = Track::ERRY + 1;

        MatXd substrate(nRows, nCols);
        substrate.col(Track::FRAME) = vFrame;
        substrate.col(Track::TIME) = vTime;

        for (uint64_t ch = 0; ch < 2; ch++)
        {
            MatXd A = iKR;
            VecXd vec = VecXd::Zero(nRows);

            for (uint64_t k = 0; k < nParticles; k++)
            {
                MatXd iK = vKernel[k];
                iK.diagonal() += avgRoute[k].col(Track::ERRX + ch);
                iK = iK.inverse();

                A += iK;
                vec += iK * avgRoute[k].col(Track::POSX + ch);

            } // loop-particles

            A = A.inverse();
            substrate.col(Track::POSX + ch) = A * vec;
            substrate.col(Track::ERRX + ch) = A.diagonal().array().sqrt();
        } // loop-dims

        return substrate;

    }

    ///////////////////////////////////////////////////////////////////////////////
    // RUN DISTRIBUTIONS //////////////////////////////////////////////////////////

    MatXd GP_FBM::distrib_singleModel(uint64_t sample_size, uint64_t id)
    {
        // Loop over all the trajectories
        runID = id;
        DA *da = singleModel(id);

        VecXd vec(4);
        vec << log(da->D), -log(2.0 / da->A - 1.0), da->mu(0), da->mu(1);

        MatXd mcmc = GOptimize::sampleParameters(vec, sample_size, &GP_FBM::weightSingle, this);

        for (uint64_t k = 0; k < sample_size; k++)
        {
            mcmc(k, 0) = exp(mcmc(k, 0)); // D

            double eA = exp(mcmc(k, 1)); // A
            mcmc(k, 1) = 2.0f * eA / (eA + 1.0f);
        }

        return mcmc;
    }

    MatXd GP_FBM::distrib_coupledModel(uint64_t sample_size)
    {
        // Generating vector with values that maximize likelihood
        CDA *cda = coupledModel();
        VecXd vec(2 * nParticles + 2);

        vec(2 * nParticles) = log(cda->DR);
        vec(2 * nParticles + 1) = -log(2.0 / cda->AR - 1.0);

        for (uint64_t k = 0; k < nParticles; k++)
        {
            vec(k) = log(cda->da[k].D);
            vec(nParticles + k) = -log(2.0 / cda->da[k].A - 1.0);
        }

        // Generate distribution
        MatXd mcmc = GOptimize::sampleParameters(vec, sample_size, &GP_FBM::weightCoupled, this);

        // Let's convert results to approppriate space
        for (uint64_t k = 0; k < sample_size; k++)
        {
            for (uint64_t l = 0; l < nParticles; l++)
            {
                mcmc(k, 2 * l) = exp(mcmc(k, 2 * l)); // D

                double val = exp(mcmc(k, 2 * l + 1));
                mcmc(k, 2 * l + 1) = 2.0f * val / (val + 1.0); // alpha
            }

            // substrate
            const uint64_t num = 2 * nParticles;
            mcmc(k, num) = exp(mcmc(k, num)); // D

            double val = exp(mcmc(k, num + 1));
            mcmc(k, num + 1) = 2.0f * val / (1.0f + val); // alpha

        }

        return mcmc;
    }

    ///////////////////////////////////////////////////////////////////////////////
    // PRIVATE FUNCTIONS

    void GP_FBM::initialize(void)
    {
        m_da.resize(nParticles);

        // Calculating the smallest timepoint accross all trajectories
        double minTime = INFINITY;
        for (MatXd &mat : route)
            minTime = std::min(mat(0, Track::TIME), minTime);

        for (MatXd &mat : route)
        {
            const uint64_t nRows = uint64_t(mat.rows());
            for (uint64_t k = 0; k < nRows; k++)
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
        const MatXd &lRoute = route[runID];
        const VecXd &vT = lRoute.col(Track::TIME);
        MatXd kernel = calcKernel(D, A, vT, vT);
        kernel.diagonal().array() += 1e-4;

        VecXd diag = kernel.diagonal();

        // LIKELIHOOD ::  The weight function is given by the negative of likelihood
        double weight = 0;
        for (uint64_t k = 0; k < 2; k++)
        {
            VecXd vec = lRoute.col(Track::POSX + k).array() - DA(2 + k);

            kernel.diagonal() = diag + lRoute.col(Track::ERRX + k);
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
        const uint64_t N = cRoute.rows();

        VecXd 
            eDA = DA.array().exp(),
            D(nParticles), A(nParticles);

        for (uint64_t k = 0; k < nParticles; k++)
        {
            D(k) = eDA[2 * k];
            A(k) = 2.0f * eDA[2 * k + 1] / (1.0f + eDA[2 * k + 1]);
        }

        double DR = eDA[2 * nParticles],
            AR = 2.0f * eDA[2 * nParticles + 1] / (eDA[2 * nParticles + 1] + 1.0);

        // Create complete kernel
        MatXd kernel(N, N);

        const uint64_t nRows = cRoute.rows() / nParticles;
        const VecXd &vt = cRoute.block(0, Track::TIME, nRows, 1);

        uint64_t sizeCol = 0, sizeRow = 0;
        for (uint64_t k = 0; k < nParticles; k++)
        {

            MatXd loc = calcKernel(D[k], A[k], vt, vt);
            MatXd RR = calcKernel(DR, AR, vt, vt);

            kernel.block(sizeRow, sizeCol, nRows, nRows) = loc + RR;

            sizeCol += nRows;
            for (uint64_t l = k + 1; l < nParticles; l++)
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
        for (uint64_t k = 0; k <= nParticles; k++)
        {
            weight -= DA(2 * k);
            weight -= (DA(2 * k + 1) - 2.0 * log(1.0 + eDA(2 * k + 1)));
        }

        return weight;

    } // weightCoupled

}
#include <gp_fbm.hpp>

#include <random>
#include <Eigen/LU>
#include <Eigen/Cholesky>

double transferSingle(const VectorXd &vec, void *ptr)
{
    return reinterpret_cast<GP_FBM *>(ptr)->weightSingle(vec);
}

double transferCoupled(const VectorXd &vec, void *ptr)
{
    return reinterpret_cast<GP_FBM *>(ptr)->weightCoupled(vec);
}

static void removeRow(MatrixXd &matrix, uint32_t rowToRemove)
{
    uint32_t numRows = matrix.rows() - 1;
    uint32_t numCols = matrix.cols();

    if (rowToRemove < numRows)
        matrix.block(rowToRemove, 0, numRows - rowToRemove, numCols) = matrix.block(rowToRemove + 1, 0, numRows - rowToRemove, numCols);

    matrix.conservativeResize(numRows, numCols);
} // removeRow

///////////////  HELPER FUNCTIONS ////////////////
static MatrixXd calcKernel(const double D, const double A,
                           const VectorXd &T1, const VectorXd &T2)
{
    const uint32_t
        NROWS = T1.size(),
        NCOLS = T2.size();

    const VectorXd
        TA1 = T1.array().pow(A),
        TA2 = T2.array().pow(A);

    MatrixXd kernel(NROWS, NCOLS);

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

static MatrixXd calcDensity(const MatrixXd &mcmc)
{
    const uint32_t nHisto = mcmc.cols(),
                   nRows = mcmc.rows();

    // Using Sturges' rule for histogram bin
    const uint32_t nPoints = 1 + ceil(log2(double(nRows))),
                   nCols = 2 * nHisto;

    MatrixXd mHisto(nPoints, nCols);
    mHisto.fill(0);

    for (uint32_t histo = 0; histo < nHisto; histo++)
    {
        // Calculate min and max values for bin width
        double minValue = INFINITY, maxValue = -INFINITY;
        for (uint32_t k = 0; k < nRows; k++)
        {
            minValue = mcmc(k, histo) < minValue ? mcmc(k, histo) : minValue;
            maxValue = mcmc(k, histo) > maxValue ? mcmc(k, histo) : maxValue;
        }

        double binWidth = (maxValue - minValue) / nPoints;

        mHisto.col(2 * histo) = VectorXd::LinSpaced(nPoints, minValue, maxValue);

        for (uint32_t k = 0; k < nRows; k++)
        {
            uint32_t id = std::min<uint32_t>((mcmc(k, histo) - minValue) / binWidth, nPoints - 1);
            mHisto(id, 2 * histo + 1)++;
        }

        double sum = mHisto.col(2 * histo + 1).sum();
        mHisto.col(2 * histo + 1).array() /= sum;

    } // loop-cols

    return mHisto;

} // calcDensity

/////////////// USER FUNCTIONS ///////////////////
GP_FBM::GP_FBM(MatrixXd &mat, Mailbox *mailbox) : mail(mailbox)
{
    if (mat.cols() > 8)
        mail->createMessage<MSG_Error>(
            "(GP_FBM) Only trajectories up to 3D are accepted!!!");

    nDims = mat.cols() == 6 ? 2 : nDims;
    nDims = mat.cols() == 8 ? 3 : nDims;

    // Inserting trajectories
    route.push_back(mat);

    // Squaring errors
    for (uint8_t ch = 0; ch < nDims; ch++)
    {
        uint32_t id = TrajColumns::ERRX + 2 * ch;
        route[0].col(id).array() *= route[0].col(id).array();
    }

    route[0].col(TrajColumns::TIME).array() -= route[0](0, TrajColumns::TIME);

    mat.col(TrajColumns::TIME).array() -= mat(0, TrajColumns::TIME);

    vecAvgTraj.resize(1);

} // constructor -- single particle

GP_FBM::GP_FBM(std::vector<MatrixXd> &mat, Mailbox *mailbox) : mail(mailbox)
{
    if (mat[0].cols() > 8)
        mail->createMessage<MSG_Error>(
            "(GP_FBM) Only trajectories up to 3D are accepted!!!");

    nDims = mat[0].cols() == 6 ? 2 : nDims;
    nDims = mat[0].cols() == 8 ? 3 : nDims;

    const uint32_t nParticles = mat.size();

    // Getting smallest time
    double minTime = INFINITY;

    // Inserting trajectories
    route.reserve(nParticles);
    for (uint32_t num = 0; num < nParticles; num++)
    {
        route.push_back(mat[num]);
        minTime = std::min<double>(mat[num](0, TrajColumns::TIME), minTime);

        // Squaring errors
        for (uint8_t ch = 0; ch < nDims; ch++)
        {
            uint32_t id = TrajColumns::ERRX + 2 * ch;
            route[num].col(id).array() *= route[num].col(id).array();
        }

    } // loop-particles

    // Bring smallest time to zero
    for (MatrixXd &oi : route)
        oi.col(TrajColumns::TIME).array() -= minTime;

    for (MatrixXd &oi : mat)
        oi.col(TrajColumns::TIME).array() -= minTime;

    vecAvgTraj.resize(nParticles);
} // constructor

const ParamDA *GP_FBM::getSingleParameters(void) const
{
    if (!bControl[SINGLE])
        mail->createMessage<MSG_Error>("(GP_FBM::getSingleParameters) Optimization must "
                                       "be done before parameters retrieval!!");

    return parSingle.data();
} // getSingleParameters

const MatrixXd *GP_FBM::getSingleDistribution(void) const
{
    if (!bControl[DTB_SINGLE])
        mail->createMessage<MSG_Error>("(GP_FBM::getSingleDistribution) No distribution "
                                       "estimated for single model!!");

    return distribSingle.data();
} // getDistirbutionSingle

void GP_FBM::setParticleID(const std::vector<std::pair<uint32_t, uint32_t>> &partID)
{
    vPartID.clear();
    vPartID.insert(vPartID.end(), partID.begin(), partID.end());
}

const ParamCDA &GP_FBM::getCoupledParameters(void) const
{
    if (!bControl[COUPLED])
        mail->createMessage<MSG_Error>("ERROR (coupled): Optimization must be done "
                                       "before parameters retrieval!!");

    return parCoupled;
} // getSingleParameters

const MatrixXd &GP_FBM::getCoupledDistribution(void) const
{
    if (!bControl[DTB_COUPLED])
        mail->createMessage<MSG_Error>("(GP_FBM::getCoupledDistribution) No "
                                       "distribution estimated for coupled model!!");

    return distribCoupled;
}

const std::vector<std::pair<uint32_t, uint32_t>> &GP_FBM::getParticleID(void) const
{
    return vPartID;
} // getParticleID

void GP_FBM::setSingleParameter(const ParamDA &par)
{
    parSingle.clear();
    parSingle.push_back(par);
    bControl.set(SINGLE);
}

void GP_FBM::setSingleParameters(const std::vector<ParamDA> &vec)
{
    parSingle.clear();
    parSingle.insert(parSingle.end(), vec.begin(), vec.end());
    bControl.set(SINGLE);
}

void GP_FBM::setCoupledParameters(const ParamCDA &par)
{
    parCoupled = par;
    bControl.set(COUPLED);
}

/////////////////  RUN MODELS ///////////////////
bool GP_FBM::singleModel(void)
{
    // Loop over all the trajectories
    for (uint32_t id = 0; id < route.size(); id++)
    {

        runId = id;
        uint32_t svec = 3;
        svec = nDims == 2 ? 4 : svec;
        svec = nDims == 3 ? 5 : svec;

        VectorXd vec(svec);
        vec(0) = log(0.5);
        vec(1) = log(0.5);

        for (uint8_t ch = 0; ch < nDims; ch++)
            vec(ch + 2) = route.at(id)(0, TrajColumns::POSX + 2 * ch);

        GOptimize::NMSimplex<double, VectorXd> nms(vec, thresSimplex);

        if (!nms.runSimplex(&transferSingle, this))
        {
            mail->createMessage<MSG_Warning>("(GP_FBM::singleModel) Single "
                                             "trajectory didn't converge!!");
            return false;
        }

        vec = nms.getResults();

        double eA = exp(vec(1));

        ParamDA p;
        p.D = exp(vec(0));
        p.A = 2.0 * eA / (eA + 1.0);

        for (uint8_t ch = 0; ch < nDims; ch++)
            p.mu[ch] = vec[2 + ch];

        parSingle.push_back(p);

    } // loop-particles

    // signal single model ran
    bControl.set(SINGLE);

    return true;

} // single model

bool GP_FBM::coupledModel(void)
{
    if (route.size() <= 1)
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::coupledModel) Cannot run "
                                         "on single trajectory!!");
        return false;
    }

    if (!bControl[SINGLE])
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::coupledModel) Particles need "
                                         "to run individually first!!");
        singleModel();
    }

    // In order to determine substrate movement, we might want to remove frames
    // that are not common to all trajectories
    const uint32_t nParticles = route.size();

    // get maximum number of frames
    uint32_t N = 0;
    for (MatrixXd &mat : route)
        N = std::max<uint32_t>(N, mat(mat.rows() - 1, TrajColumns::FRAME) + 1);

    // check if frame is in all movies
    std::vector<uint32_t> vCount(N, 0);
    for (MatrixXd &mat : route)
        for (uint32_t k = 0; k < mat.rows(); k++)
        {
            uint32_t fr = mat(k, TrajColumns::FRAME);
            vCount[fr]++;
        }

    for (MatrixXd &mat : route)
        for (int k = mat.rows() - 1; k >= 0; k--)
        {
            uint32_t fr = mat(k, TrajColumns::FRAME);
            if (vCount[fr] != nParticles)
                removeRow(mat, k);
        }

    // Generate concatenated matrix
    uint32_t ct = 0;
    for (MatrixXd &mat : route)
        ct += mat.rows();

    const uint32_t nCols = route[0].cols();
    cRoute = MatrixXd(ct, nCols);

    // Creating initial position

    ct = 0;
    VectorXd vec(2 * nParticles + 2);
    for (uint32_t k = 0; k < nParticles; k++)
    {
        ParamDA &p = parSingle.at(k);
        vec(2 * k) = log(p.D);
        vec(2 * k + 1) = -log(2.0 / p.A - 1.0);

        MatrixXd loc = route.at(k);
        for (uint8_t ch = 0; ch < nDims; ch++)
            loc.col(TrajColumns::POSX + 2 * ch).array() -= p.mu[ch];

        cRoute.block(ct, 0, loc.rows(), nCols) = loc;
        ct += loc.rows();
    }

    if (ct < nParticles * 50)
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::coupledModel) Not all "
                                         "trajectories intersect!!");
        return false;
    }

    vec(2 * nParticles + 0) = 0.0; //DR
    vec(2 * nParticles + 1) = 0.0; // AR

    GOptimize::NMSimplex<double, VectorXd> nms(vec, thresSimplex);
    if (!nms.runSimplex(&transferCoupled, this))
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::coupledModel) Model didn't converge!");
        return false;
    }

    // Adapting outputs to appropriate space
    vec = nms.getResults().array().exp();

    parCoupled.DR = vec(2 * nParticles);
    parCoupled.AR = 2.0f * vec(2 * nParticles + 1) / (1.0f + vec(2 * nParticles + 1));

    for (uint32_t k = 0; k < nParticles; k++)
    {
        ParamDA p = parSingle.at(k);
        p.D = vec(2 * k);
        p.A = 2.0 * vec(2 * k + 1) / (1.0 + vec(2 * k + 1));
        parCoupled.particle.push_back(p);
    }

    // signaling coupled model ran
    bControl.set(COUPLED);

    return true;
} // coupledModel

////////////// ESTIMATE TRAJECTORIES  /////////////////////

void GP_FBM::calcAvgTrajectory(const VectorXd &vFrame, const VectorXd &vTime,
                               const uint32_t id)
{
    if (id >= route.size())
        mail->createMessage<MSG_Error>("(GP_FBM::calcAvgTrajectory) id doesn't exist!!");

    // Create output matrix
    MatrixXd traj(vTime.size(), route[id].cols());
    traj.col(TrajColumns::FRAME) = vFrame;
    traj.col(TrajColumns::TIME) = vTime;

    // Getting particle's dynamical parameters
    const double
        D = parSingle[id].D,
        A = parSingle[id].A;

    // Importing trajectory
    MatrixXd mat = route[id];
    for (uint32_t k = 0; k < nDims; k++)
        mat.col(TrajColumns::POSX + 2 * k).array() -= parSingle[id].mu[k];

    const VectorXd &VT = mat.col(TrajColumns::TIME);

    MatrixXd
        K = calcKernel(D, A, VT, VT),
        Ks = calcKernel(D, A, VT, vTime),
        KsT = Ks.transpose(),
        K2s = calcKernel(D, A, vTime, vTime);

    VectorXd diagK = K.diagonal();

    for (uint8_t ch = 0; ch < nDims; ch++)
    {
        K.diagonal() = diagK + route[id].col(TrajColumns::ERRX + 2 * ch);

        // Calculating particle's expected position
        Eigen::LLT<MatrixXd> cholesky = K.llt();
        VectorXd alpha = cholesky.solve(mat.col(TrajColumns::POSX + 2 * ch));

        traj.col(TrajColumns::POSX + 2 * ch) = KsT * alpha;
        traj.col(TrajColumns::POSX + 2 * ch).array() += parSingle[id].mu[ch];

        // Calculating positioning expected error

        const VectorXd ver1 = K2s.diagonal();
        const VectorXd ver2 = (KsT * cholesky.solve(Ks)).diagonal();

        traj.col(TrajColumns::ERRX + 2 * ch) = (ver1 - ver2).array().sqrt();
        traj(0, TrajColumns::ERRX + 2 * ch) = sqrt(route[id](0, TrajColumns::ERRX + 2 * ch));
    }

    vecAvgTraj[id] = traj;

} // dataTreatment

void GP_FBM::estimateSubstrateMovement(void)
{
    if (!bControl[COUPLED])
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::estimateSubstrateMovement) "
                                         "'coupledModel' must run before "
                                         "estimateSubstrateMovement'!!");
        if (!coupledModel())
        {
            mail->createMessage<MSG_Error>("(GP_FBM::estimateSubstrateMovement) "
                                           "'coupledModel' didn't converge!");
            return;
        }
    }
    // Let's determine time points which we need to calculate substrate movement
    // Determine maximum number of frames
    uint32_t maxFR = 0;
    for (auto &mat : route)
    {
        const uint32_t k = mat.rows() - 1;
        maxFR = std::max<uint32_t>(maxFR, mat(k, TrajColumns::FRAME));
    }

    // Check all frames present
    std::vector<std::pair<bool, double>> vfr(maxFR + 1);
    for (auto &mat : route)
    {
        const uint32_t N = mat.rows();
        for (uint32_t k = 0; k < N; k++)
            vfr[int(mat(k, 0))] = {true, mat(k, TrajColumns::TIME)};
    }

    // We use those present
    std::vector<double> auxFrame, auxTime;
    for (uint32_t k = 0; k < vfr.size(); k++)
        if (vfr[k].first)
        {
            auxFrame.push_back(k);
            auxTime.push_back(vfr[k].second);
        }

    // Let's calculate average trajectories with precision
    VectorXd vFrame = VectorXd::Map(auxFrame.data(), auxFrame.size());
    VectorXd vTime = VectorXd::Map(auxTime.data(), auxTime.size());

    std::vector<MatrixXd> avgRoute;
    for (uint32_t k = 0; k < route.size(); k++)
    {
        calcAvgTrajectory(vFrame, vTime, k);
        MatrixXd mat = getAvgTrajectory(k);
        for (uint8_t ch = 0; ch < nDims; ch++)
        {
            // We will need variance for next steps
            mat.col(TrajColumns::ERRX + 2 * ch).array() *=
                mat.col(TrajColumns::ERRX + 2 * ch).array();

            // We should subtract average position to estimate substrate movement
            mat.col(TrajColumns::POSX + 2 * ch).array() -= parSingle[k].mu[ch];
        }

        avgRoute.emplace_back(std::move(mat));
    }

    // We shall calculate all kernels now
    std::vector<MatrixXd> vKernel(route.size());
    for (uint32_t k = 0; k < route.size(); k++)
    {
        const double
            D = parCoupled.particle[k].D,
            A = parCoupled.particle[k].A;

        vKernel[k] = calcKernel(D, A, vTime, vTime);
    }

    // Kernel for substrate
    MatrixXd iKR = calcKernel(parCoupled.DR, parCoupled.AR, vTime, vTime);
    iKR.diagonal().array() += 0.0001;
    iKR = iKR.inverse();

    // Let's estimate substrate movement
    substrate = MatrixXd(vTime.size(), 2 * nDims + TrajColumns::POSX);
    substrate.col(TrajColumns::FRAME) = vFrame;
    substrate.col(TrajColumns::TIME) = vTime;

    for (uint8_t ch = 0; ch < nDims; ch++)
    {
        MatrixXd A = iKR;
        VectorXd vec = VectorXd::Zero(vTime.size());

        for (uint32_t k = 0; k < route.size(); k++)
        {
            MatrixXd iK = vKernel[k];
            iK.diagonal() += avgRoute[k].col(TrajColumns::ERRX + 2 * ch);
            iK = iK.inverse();

            A += iK;
            vec += iK * avgRoute[k].col(TrajColumns::POSX + 2 * ch);

        } // loop-particles

        A = A.inverse();
        substrate.col(TrajColumns::POSX + 2 * ch) = A * vec;
        substrate.col(TrajColumns::ERRX + 2 * ch) = A.diagonal().array().sqrt();
    } // loop-dims

    bControl.set(SUBSTRATE);

} // estimateSubstrateMovement

///////////////// MODELS ///////////////////////
double GP_FBM::weightSingle(const VectorXd &DA)
{
    // Rescaling parameters to proper values
    double
        D = exp(DA(0)),
        eA = exp(DA(1)),
        A = 2.0 * eA / (eA + 1.0);

    // Building FBM kernel
    const VectorXd &vT = route.at(runId).col(TrajColumns::TIME);
    MatrixXd kernel = calcKernel(D, A, vT, vT);
    kernel.diagonal().array() += 1e-4;

    VectorXd diag = kernel.diagonal();

    // LIKELIHOOD ::  The weight function is given by the negative of likelihood
    double weight = 0;
    for (uint8_t ch = 0; ch < nDims; ch++)
    {
        VectorXd vec = route.at(runId).col(TrajColumns::POSX + 2 * ch).array() - DA(2 + ch);

        kernel.diagonal() = diag + route.at(runId).col(TrajColumns::ERRX + 2 * ch);
        Eigen::LLT<MatrixXd> cholesky = kernel.llt();
        VectorXd alpha = cholesky.solve(vec);
        MatrixXd L = cholesky.matrixL();
        weight += 0.5 * vec.dot(alpha) + L.diagonal().array().log().sum();
    }

    // flat prior on A
    return weight - DA(1) + 2.0 * log(1.0 + eA) - DA(0);

} //weightSingle

double GP_FBM::weightCoupled(const VectorXd &DA)
{
    const uint32_t N = cRoute.rows(),
                   nParticles = route.size();

    VectorXd eDA = DA.array().exp(),
             D(nParticles), A(nParticles);

    for (uint32_t k = 0; k < nParticles; k++)
    {
        D(k) = eDA[2 * k];
        A(k) = 2.0f * eDA[2 * k + 1] / (1.0f + eDA[2 * k + 1]);
    }

    double DR = eDA[2 * nParticles],
           AR = 2.0f * eDA[2 * nParticles + 1] / (eDA[2 * nParticles + 1] + 1.0);

    // Create complete kernel
    MatrixXd kernel(N, N);
    uint32_t sizeCol = 0, sizeRow = 0;
    for (uint32_t k = 0; k < nParticles; k++)
    {
        const uint32_t n1 = route[k].rows();
        const VectorXd &vt1 = route[k].col(TrajColumns::TIME);

        kernel.block(sizeRow, sizeCol, n1, n1) = calcKernel(D[k], A[k], vt1, vt1) +
                                                 calcKernel(DR, AR, vt1, vt1);

        sizeCol += n1;
        for (uint32_t l = k + 1; l < nParticles; l++)
        {

            const uint32_t n2 = route[l].rows();
            const VectorXd &vt2 = route[l].col(TrajColumns::TIME);
            MatrixXd mat = calcKernel(DR, AR, vt1, vt2);

            kernel.block(sizeRow, sizeCol, n1, n2) = mat;
            kernel.block(sizeCol, sizeRow, n2, n1) = mat.transpose();
            sizeCol += n2;
        }

        sizeRow += n1;
        sizeCol = sizeRow;
    } // loop-particles

    // To avoid to problem with determinant zero, let's add a minor variance to system
    kernel.diagonal().array() += 1e-4;

    // We'll need the diagonal later
    VectorXd diag = kernel.diagonal();

    /////////////////////////////////////////////////////////////////
    // LIKELIHOOD
    // The weight function is given by the negative of likelihood

    double weight = 0;
    for (uint8_t ch = 0; ch < nDims; ch++)
    {
        const VectorXd &vec = cRoute.col(TrajColumns::POSX + 2 * ch);

        // X direction
        kernel.diagonal() = diag + cRoute.col(TrajColumns::ERRX + 2 * ch);
        Eigen::LLT<MatrixXd> cholesky = kernel.llt();
        VectorXd alpha = cholesky.solve(vec);
        MatrixXd L = cholesky.matrixL();
        weight += 0.5f * vec.dot(alpha);
        weight += L.diagonal().array().log().sum();
    }

    // Flat priors for all A
    for (uint32_t k = 0; k <= nParticles; k++)
    {
        weight -= DA(2 * k);
        weight -= (DA(2 * k + 1) - 2.0 * log(1.0 + eDA(2 * k + 1)));
    }

    return weight;

} // weightCorrelated

/////////////////  RUN DISTRIBUTIONS ///////////////////
void GP_FBM::distrib_singleModel(void)
{
    if (!bControl[SINGLE])
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::distrib_singleModel) We need to "
                                         "run optimizer before distribution!!");

        if (!singleModel())
        {
            mail->createMessage<MSG_Error>("(GP_FBM::distrib_singleModel) 'singleModel' "
                                           "didn't converge!");
            return;
        }
    }

    // Loop over all the trajectories
    for (uint32_t id = 0; id < route.size(); id++)
    {
        runId = id;
        ParamDA &p = parSingle.at(id);

        uint32_t svec = 3;
        svec = nDims == 2 ? 4 : svec;
        svec = nDims == 3 ? 5 : svec;

        VectorXd vec(svec);
        vec(0) = log(p.D);
        vec(1) = -log(2.0 / p.A - 1.0);

        for (uint8_t ch = 0; ch < nDims; ch++)
            vec(ch + 2) = p.mu[ch];

        MatrixXd mcmc =
            GOptimize::sampleParameters<double, VectorXd, MatrixXd>(
                vec, maxMCS, &transferSingle, this);

        for (uint32_t k = 0; k < maxMCS; k++)
        {
            mcmc(k, 0) = exp(mcmc(k, 0)); // D

            double eA = exp(mcmc(k, 1)); // A
            mcmc(k, 1) = 2.0f * eA / (eA + 1.0f);
        }

        // Appending to class variable
        distribSingle.push_back(calcDensity(mcmc));
    } // loop-particles

    // Signaling distribution was calculated
    bControl.set(DTB_SINGLE);

} // distrib_singleModel

void GP_FBM::distrib_coupledModel(void)
{
    if (!bControl[COUPLED])
    {
        mail->createMessage<MSG_Warning>("(GP_FBM::distrib_coupledModel) Must run "
                                         "coupled optimization first!!");

        if (!coupledModel())
        {
            mail->createMessage<MSG_Error>("(GP_FBM::distrib_coupledModel) "
                                           "'coupledModel' didn't converge!!");
            return;
        }
    }

    // Generating vector with values that maximize likelihood
    const uint32_t nParticles = route.size();
    VectorXd vec(2 * nParticles + 2);

    vec(2 * nParticles) = log(parCoupled.DR);
    vec(2 * nParticles + 1) = -log(2.0 / parCoupled.AR - 1.0);

    for (uint32_t k = 0; k < nParticles; k++)
    {
        ParamDA &p = parCoupled.particle.at(k);
        vec(k) = log(p.D);
        vec(nParticles + k) = -log(2.0 / p.A - 1.0);
    }

    // Generate distribution
    MatrixXd mcmc = GOptimize::sampleParameters<double, VectorXd, MatrixXd>(
        vec, maxMCS, &transferCoupled, this);

    // Let's convert results to approppriate space
    for (uint32_t k = 0; k < maxMCS; k++)
    {
        for (uint32_t l = 0; l < nParticles; l++)
        {
            mcmc(k, 2 * l) = expf(mcmc(k, 2 * l)); // D

            double val = expf(mcmc(k, 2 * l + 1));
            mcmc(k, 2 * l + 1) = 2.0f * val / (val + 1.0f); // alpha
        }

        // substrate
        const uint32_t num = 2 * nParticles;
        mcmc(k, num) = expf(mcmc(k, num)); // D

        double val = expf(mcmc(k, num + 1));
        mcmc(k, num + 1) = 2.0f * val / (1.0f + val); // alpha

    } // loop-rows

    distribCoupled = calcDensity(mcmc);

    // Signaling distribution was done
    bControl.set(DTB_COUPLED);

} // distrib_coupledModel

#include <gtest/gtest.h>
#include <fstream>

#include "GPMethods.h"
#include "json/json.h"


static void removeRow(MatXd& mat, uint64_t row)
{
    uint64_t
        nRows = mat.rows() - 1,
        nCols = mat.cols();

    if (row < nRows)
        mat.block(row, 0, nRows - row, nCols) = mat.block(row + 1, 0, nRows - row, nCols);

    mat.conservativeResize(nRows, nCols);
}

static MatXd genKernel(double D, double A, const VecXd& vt)
{
    VecXd vta = vt.array().pow(A);
    MatXd K(vt.size(), vt.size());

    for (int64_t k = 0; k < vt.size(); k++)
        for (int64_t l = k; l < vt.size(); l++)
        {
             double val = vta(k) + vta(l) - pow(abs(vt(k) - vt(l)), A);
            K(k, l) = K(l, k) = D * val;
        }

    return K;
}

static MatXd genTrajectory(const MatXd& kernel, const VecXd& vframe, const VecXd& vtime, double error)
{
    std::random_device dev;
    std::default_random_engine ran(dev());
    std::normal_distribution<double> normal(0.0, 1.0);

    int64_t N = vtime.size();

    MatXd unit(N, 2);
    for (int64_t k = 0; k < unit.size(); k++)
        unit.data()[k] = normal(ran);


    Eigen::LLT<MatXd> cholesky(kernel); // waste of energy, but whatever

    MatXd out(N, 6);
    out.col(0) = vframe;
    out.col(1) = vtime;
    out.block(0, 2, N, 2) = cholesky.matrixL() * unit;
    out.block(0, 4, N, 2).array() = error;

    return out;
}

///////////////////////////////////////////////////////////
// Main tests


TEST(GP_FBM, parameterInference_Single)
{
    uint64_t
        numSteps = 200,
        sampleSize = 100;

    double
        timeStep = 0.5,
        error = 0.05,
        D = 0.1, A = 0.45;

    const int32_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> vThr(numThreads);


    VecXd 
        vframe = VecXd::LinSpaced(numSteps, 0.0, double(numSteps) - 1.0),
        vtime = timeStep * vframe;

    MatXd
        K1 = genKernel(D, A, vtime) + error * error * MatXd::Identity(numSteps, numSteps),
        single(sampleSize, 2);

    auto parallelFunction = [&](const int32_t threadId, const int32_t numThreads) -> void {
        for (int32_t sid = threadId; sid < sampleSize; sid += numThreads)
        {
            MatXd traj = genTrajectory(K1, vframe, vtime, error);

            GPT::GP_FBM gp(traj);
            GPT::GP_FBM::DA* da = gp.singleModel(0);
            single.row(sid) << da->D, da->A;
        }
    };


    for (int32_t k = 0; k < numThreads; k++)
        vThr[k] = std::thread(parallelFunction, k, numThreads);

    for (std::thread& thr : vThr)
        thr.join();

    // Runnning some tests
    double mu, dev, score;
    mu = single.col(0).mean();
    dev = sqrt((single.col(0).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - D);
    EXPECT_LE(score, 0.1) << "GP_FBM :: single D is incorrect";

    mu = single.col(1).mean();
    dev = sqrt((single.col(1).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - A);
    EXPECT_LE(score, 0.1) << "GP_FBM :: single A is incorrect";


    single.col(0).array() -= D;
    single.col(1).array() -= A;

    // Saving results
    std::ofstream arq(fs::path(GTEST_PATH) / "singleGP.txt");
    arq << single;
    arq.close();
}



TEST(GP_FBM, parameterInference_Coupled)
{
    uint64_t
        numSteps = 200,
        sampleSize = 100;

    double
        timeStep = 0.5,
        error = 0.05,
        D1 = 0.1, A1 = 0.45,
        D2 = 0.08, A2 = 0.5,
        DR = 0.5, AR = 1.0;

    const int32_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> vThr(numThreads);

    VecXd 
        vframe = VecXd::LinSpaced(numSteps, 0.0, double(numSteps) - 1.0),
        vtime = timeStep * vframe;

    MatXd
        K1 = genKernel(D1, A1, vtime) + error * error * MatXd::Identity(numSteps, numSteps),
        K2 = genKernel(D2, A2, vtime) + error * error * MatXd::Identity(numSteps, numSteps),
        KR = genKernel(DR, AR, vtime) + 1e-6 * MatXd::Identity(numSteps, numSteps);

   
    MatXd sample(sampleSize, 6);

    auto parallelFunction = [&](const int32_t threadId, const int32_t numThreads) -> void {
        for (int32_t sid = threadId; sid < sampleSize; sid += numThreads)
        {
            MatXd
                traj1 = genTrajectory(K1, vframe, vtime, error),
                traj2 = genTrajectory(K2, vframe, vtime, error),
                trajR = genTrajectory(KR, vframe, vtime, error);


            for (uint64_t k = 2; k < 4; k++)
            {
                traj1.col(k) += trajR.col(k);
                traj2.col(k) += trajR.col(k);
            }

            GPT::GP_FBM gp(std::vector<MatXd>{ traj1, traj2 });
            GPT::GP_FBM::CDA* cda = gp.coupledModel();
            sample.row(sid) << cda->da[0].D, cda->da[1].D, cda->DR, cda->da[0].A, cda->da[1].A, cda->AR;
        }
    };

    for (int32_t k = 0; k < numThreads; k++)
        vThr[k] = std::thread(parallelFunction, k, numThreads);

    for (std::thread& thr : vThr)
        thr.join();


    double mu, dev, score;
    // Running some simple tests :: Diffusion coefficient
    mu = sample.col(0).mean();
    dev = sqrt((sample.col(0).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - D1);
    EXPECT_LE(score, 0.1) << "GP_FBM :: coupled D1 is incorrect";

    mu = sample.col(1).mean();
    dev = sqrt((sample.col(1).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - D2);
    EXPECT_LE(score, 0.1) << "GP_FBM :: coupled D2 is incorrect";

    mu = sample.col(2).mean();
    dev = sqrt((sample.col(2).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - DR);
    EXPECT_LE(score, 0.1) << "GP_FBM :: coupled DR is incorrect";

    // Running some simple tests :: Anomalous coefficent
    mu = sample.col(3).mean();
    dev = sqrt((sample.col(3).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - A1);
    EXPECT_LE(score, 0.1) << "GP_FBM :: coupled A1 is incorrect";

    mu = sample.col(4).mean();
    dev = sqrt((sample.col(4).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - A2);
    EXPECT_LE(score, 0.1) << "GP_FBM :: coupled A2 is incorrect";

    mu = sample.col(5).mean();
    dev = sqrt((sample.col(5).array() - mu).square().mean() / double(sampleSize));
    score = abs(mu - AR);
    EXPECT_LE(score, 0.1) << "GP_FBM :: coupled AR is incorrect";


    sample.col(0).array() -= D1;
    sample.col(1).array() -= D2;
    sample.col(2).array() -= DR;

    sample.col(3).array() -= A1;
    sample.col(4).array() -= A2;
    sample.col(5).array() -= AR;


    // Saving some results for ploting
    std::ofstream arq(fs::path(GTEST_PATH) / "coupledGP.txt");
    arq << sample;
    arq.close();
}



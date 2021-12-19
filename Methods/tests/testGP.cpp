#include "testHeader.h"

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

    // Running tests on percents
    single.col(0).array() = (single.col(0).array() - D) / D;
    single.col(1).array() = (single.col(1).array() - A) / A;

    // Runnning some tests
    double mu;
    mu = abs(single.col(0).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: single D is incorrect";

    mu = abs(single.col(1).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: single A is incorrect";


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


    // Let's convert into percents
    sample.col(0).array() = (sample.col(0).array() - D1) / D1;
    sample.col(1).array() = (sample.col(1).array() - D2) / D2;
    sample.col(2).array() = (sample.col(2).array() - DR) / DR;

    sample.col(3).array() = (sample.col(3).array() - A1) / A1;
    sample.col(4).array() = (sample.col(4).array() - A2) / A2;
    sample.col(5).array() = (sample.col(5).array() - AR) / AR;


    double mu;
    // Running some simple tests :: Diffusion coefficient :: Average error less than 10%
    mu = abs(sample.col(0).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: coupled D1 is incorrect";
    mu = abs(sample.col(1).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: coupled D2 is incorrect";
    mu = abs(sample.col(2).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: coupled DR is incorrect";

    // Running some simple tests :: Anomalous coefficent :: Average less than 10%
    mu = abs(sample.col(3).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: coupled A1 is incorrect";
    mu = abs(sample.col(4).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: coupled A2 is incorrect";
    mu = abs(sample.col(5).mean());
    EXPECT_LE(mu, 0.05) << "GP_FBM :: coupled AR is incorrect";


    // Saving some results for ploting
    std::ofstream arq(fs::path(GTEST_PATH) / "coupledGP.txt");
    arq << sample;
    arq.close();
}


TEST(GP_FBM, trajectoryInterpolation)
{
    uint64_t numSteps = 200;

    double
        timeStep = 0.5,
        error = 0.05,
        D = 0.1, A = 0.45;


    VecXd
        vframe = VecXd::LinSpaced(numSteps, 0.0, double(numSteps) - 1.0),
        vtime = timeStep * vframe;

    MatXd 
        K1 = genKernel(D, A, vtime) + error * error * MatXd::Identity(numSteps, numSteps),
        traj = genTrajectory(K1, vframe, vtime, error);
        

    GPT::GP_FBM gp(traj);
    MatXd avg = gp.calcAvgTrajectory(vtime);

    MatXd diff = avg.block(0, GPT::Track::POSX, avg.rows(), 2) - traj.block(0, GPT::Track::POSX, traj.rows(), 2);

    EXPECT_LE(abs(diff.mean()), 0.001) << "GP_FBM :: Interpolation differences are too big";
}


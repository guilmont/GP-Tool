#include "testHeader.h"


TEST(Batching, generatesDataForBatchingTest_Single)
{
    uint64_t
        iHeight = 64,
        iWidth = 64,

        numSteps = 150,
        sampleSize = 100;

    const double
        timeStep = 0.5, // We will need to rescale for prints
        error = 0.001,
        D1 = 0.1, 
        A1 = 0.5,
        signal = 150,
        bGround = 100,
        spotSize = 2;


    fs::path outPath = GTEST_SINGLE;
    using Mat16 = Image<uint16_t>;


    const int32_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> vThr(numThreads);

    VecXd
        vframe = VecXd::LinSpaced(numSteps, 0.0, double(numSteps) - 1.0),
        vtime = timeStep * vframe;

    MatXd K1 = genKernel(D1, A1, vtime) + error * error * MatXd::Identity(numSteps, numSteps);


    auto parallelFunction = [&](const int32_t threadId, const int32_t numThreads) -> void {
        for (int32_t sid = threadId; sid < sampleSize; sid += numThreads)
        {
            std::string name = "test" + std::to_string(sid);

            MatXd traj = genTrajectory(K1, vframe, vtime, error);

            traj.col(1) = traj.col(GPT::Track::FRAME);
            traj.col(0).array() = 0.0;
            traj.col(GPT::Track::POSX).array() += 0.5 * double(iWidth);
            traj.col(GPT::Track::POSY).array() += 0.5 * double(iHeight);

            saveTrajCSV(outPath / (name + "_traj.csv"), traj.block(0,0, numSteps, 4));

            // Generating movies
            MatXd particles(1, 2);
            std::vector<Mat16> vImg;

            for (uint64_t k = 0; k < numSteps; k++)
            {
                particles(0, 0) = traj(k, GPT::Track::POSX);
                particles(0, 1) = traj(k, GPT::Track::POSY);

                MatXd img = generateImage(iHeight, iWidth, signal, bGround, spotSize, particles);
                poissonNoise(img);
                vImg.push_back(img.cast<uint16_t>());
            }

            GPT::Tiffer::Write writer(vImg);
            writer.save(outPath / (name + ".tif"));

        }
    };

    for (int32_t k = 0; k < numThreads; k++)
        vThr[k] = std::thread(parallelFunction, k, numThreads);

    for (std::thread& thr : vThr)
        thr.join();


}

TEST(Batching, generatesDataForBatchingTest_Coupled)
{
    uint64_t
        iHeight = 64,
        iWidth = 64,
        
        numSteps = 150,
        sampleSize = 100;

    const double
        timeStep = 0.5, // We will need to rescale for plots
        error = 0.001,
        D1 = 0.1, A1 = 0.45,
        D2 = 0.08, A2 = 0.5,
        DR = 0.5, AR = 1.0,
    
        signal = 150,
        bGround = 100,
        spotSize = 2;


    fs::path outPath = GTEST_COUPLED;
    using Mat16 = Image<uint16_t>;


    const int32_t numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> vThr(numThreads);

    VecXd 
        vframe = VecXd::LinSpaced(numSteps, 0.0, double(numSteps) - 1.0),
        vtime = timeStep * vframe;

    MatXd
        K1 = genKernel(D1, A1, vtime) + error * error * MatXd::Identity(numSteps, numSteps),
        K2 = genKernel(D2, A2, vtime) + error * error * MatXd::Identity(numSteps, numSteps),
        KR = genKernel(DR, AR, vtime) + 1e-6 * MatXd::Identity(numSteps, numSteps);

 

    auto parallelFunction = [&](const int32_t threadId, const int32_t numThreads) -> void {
        for (int32_t sid = threadId; sid < sampleSize; sid += numThreads)
        {
            std::string name = "test" + std::to_string(sid);

            MatXd
                traj1 = genTrajectory(K1, vframe, vtime, error),
                traj2 = genTrajectory(K2, vframe, vtime, error),
                trajR = genTrajectory(KR, vframe, vtime, error);


            for (uint64_t k = GPT::Track::POSX; k <= GPT::Track::POSY; k++)
            {
                traj1.col(k) += trajR.col(k);
                traj2.col(k) += trajR.col(k);
            }

            // Saving file with trajectories
            MatXd trajData(2 * numSteps, 4);
            trajData.block(0, 0, numSteps, 1).array() = 0;
            trajData.block(0, 1, numSteps, 1) = traj1.col(GPT::Track::FRAME);
            trajData.block(0, 2, numSteps, 1) = traj1.block(0, GPT::Track::POSX, numSteps, 1).array() + 0.4 * double(iWidth);
            trajData.block(0, 3, numSteps, 1) = traj1.block(0, GPT::Track::POSY, numSteps, 1).array() + 0.5 * double(iHeight);

            trajData.block(numSteps, 0, numSteps, 1).array() = 1;
            trajData.block(numSteps, 1, numSteps, 1) = traj2.col(GPT::Track::FRAME);
            trajData.block(numSteps, 2, numSteps, 1) = traj2.block(0, GPT::Track::POSX, numSteps, 1).array() + 0.6 * double(iWidth);
            trajData.block(numSteps, 3, numSteps, 1) = traj2.block(0, GPT::Track::POSY, numSteps, 1).array() + 0.5 * double(iHeight);


            saveTrajCSV(outPath / (name + "_traj.csv"), trajData);

            // Generating movies
            MatXd particles(2, 2);
            std::vector<Mat16> vImg;

            for (uint64_t k = 0; k < numSteps; k++)
            {
                particles(0, 0) = traj1(k, GPT::Track::POSX) + 0.4 * double(iWidth);
                particles(0, 1) = traj1(k, GPT::Track::POSY) + 0.5 * double(iHeight);
                
                particles(1, 0) = traj2(k, GPT::Track::POSX) + 0.6 * double(iWidth);
                particles(1, 1) = traj2(k, GPT::Track::POSY) + 0.5 * double(iHeight);

                MatXd img = generateImage(iHeight, iWidth, signal, bGround, spotSize, particles);
                poissonNoise(img);
                vImg.push_back(img.cast<uint16_t>());
            }

            GPT::Tiffer::Write writer(vImg);
            writer.save(outPath / (name + ".tif"));

        }
    };

    for (int32_t k = 0; k < numThreads; k++)
        vThr[k] = std::thread(parallelFunction, k, numThreads);

    for (std::thread& thr : vThr)
        thr.join();

}




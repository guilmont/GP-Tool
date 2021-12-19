#include "testHeader.h"

TEST(Images, alignment)
{
    const fs::path outPath = GTEST_PATH;

    const uint64_t
        nParticles = 100,
        iHeight = 512,
        iWidth = 512;

    const double
        signal = 150,
        bGround = 100,
        spotSize = 5;


    // Generating particles for image
    std::random_device dev;
    std::default_random_engine ran(dev());
    std::uniform_real_distribution<double> unif(20.0, 492.0);
    
    MatXd particles(nParticles, 2);
    for (uint64_t k = 0; k < nParticles; k++)
    {
        uint64_t counter = 0;
        while (counter++ < 10 * nParticles) // not ideal, but there are few positions
        {
            double x = unif(ran), y = unif(ran);

            bool good = true;
            for (uint64_t l = 0; l < k; l++)
                if (abs(particles(l, 0) - x) < 5.0 * spotSize && abs(particles(l, 1) - y) < 5.0 * spotSize)
                {
                    good = false;
                    break;
                }

            if (good)
            {
                particles(k, 0) = x;
                particles(k, 1) = y;
                break;
            }
        }

        ASSERT_GT(10 * nParticles, counter) << "Too many loops to generate particles"; // Just in case
    }


    // Generating original image
    MatXd image = generateImage(iHeight, iWidth, signal, bGround, spotSize, particles);


    // Generate rotated image
    Mat3d trf = transformMatrix(iWidth, iHeight);
    Mat3d itrf = trf.inverse();

    MatXd rotated = bGround * MatXd::Ones(iHeight, iWidth);
    for (uint64_t k = 0; k < iHeight; k++)
        for (uint64_t l = 0; l < iWidth; l++)
        {
            int64_t col = int64_t(itrf(0, 0) * (double(l) + 0.5) + itrf(0, 1) * (double(k) + 0.5) + itrf(0, 2));
            int64_t row = int64_t(itrf(1, 0) * (double(l) + 0.5) + itrf(1, 1) * (double(k) + 0.5) + itrf(1, 2));

            if (row >= 0 && row < iHeight && col >= 0 && col < iWidth)
                rotated(k, l) = image(row, col);
        }

    // To make a bit more interesting, let's add some Poisson noise
    poissonNoise(image);
    poissonNoise(rotated);

    // Let's calculated corrections
    GPT::Align align(1, std::vector<MatXd>{image}.data(), std::vector<MatXd>{rotated}.data());
    ASSERT_EQ(true, align.alignCameras());
    ASSERT_EQ(true, align.correctAberrations());
    
    const GPT::TransformData& RT = align.getTransformData();


    // Let's calculated the corrected iamge
    MatXd corrected = MatXd::Zero(iHeight, iWidth);
    for (int64_t k = 0; k < corrected.rows(); k++)
        for (int64_t l = 0; l < corrected.cols(); l++)
        {
            int64_t x = int64_t(RT.itrf(0, 0) * (double(l) + 0.5) + RT.itrf(0, 1) * (double(k) + 0.5) + RT.itrf(0, 2));
            int64_t y = int64_t(RT.itrf(1, 0) * (double(l) + 0.5) + RT.itrf(1, 1) * (double(k) + 0.5) + RT.itrf(1, 2));

            if (y >= 0 && y < corrected.rows() && x >= 0 && x < corrected.cols())
                corrected(k, l) = rotated(y, x);
        }

    // Calculating rotated and corrected positions
    MatXd posRotated(nParticles, 2), posCorrected(nParticles, 2);
    for (uint64_t k = 0; k < nParticles; k++)
    {
        posRotated(k, 0) = trf(0, 0) * particles(k, 0) + trf(0, 1) * particles(k, 1) + trf(0, 2);
        posRotated(k, 1) = trf(1, 0) * particles(k, 0) + trf(1, 1) * particles(k, 1) + trf(1, 2);

        posCorrected(k,0) =  RT.trf(0, 0) * posRotated(k, 0) + RT.trf(0, 1) * posRotated(k, 1) + RT.trf(0, 2);
        posCorrected(k,1) = RT.trf(1, 0) * posRotated(k, 0) + RT.trf(1, 1) * posRotated(k, 1) + RT.trf(1, 2);
    }

    

    // Saving everything to a file make some nice plots
    Json::Value output;

    output["Original"]["Image"] = jsonEigen(image);
    output["Original"]["Positions"] = jsonEigen(particles);

    output["Rotated"]["Image"] = jsonEigen(rotated);
    output["Rotated"]["Positions"] = jsonEigen(posRotated);

    output["Corrected"]["Image"] = jsonEigen(corrected);
    output["Corrected"]["Positions"] = jsonEigen(posCorrected);

    std::ofstream arq(outPath / "testAlignment.json");
    arq << output;
    arq.close();


    //////////////////////////////////////////////////////
    // Running some basic tests here

    posCorrected -= particles;
    posRotated -= particles;

    VecXd cor = (posCorrected.col(0).array().square() + posCorrected.col(1).array().square()).sqrt();
    VecXd rot = (posRotated.col(0).array().square() + posRotated.col(1).array().square()).sqrt();


    double muRot = rot.mean(), varRot = rot.array().square().mean();
    double muCor = cor.mean(), varCor = cor.array().square().mean();

    EXPECT_LT(muCor, muRot) << " Alignment :: Corrected avg distances are greater then raw ones";

    double zscore = (muRot - muCor) / sqrt((varRot + varCor) / double(nParticles));
    EXPECT_GE(abs(zscore), 3.0) << "Alignement :: Correction is not significant";

}

TEST(Images, enhancement)
{
    const fs::path outPath = GTEST_PATH;

    const uint64_t
        nImages = 100,
        iHeight = 16,
        iWidth = 16;

    const double
        signal = 150,
        bGround = 100,
        spotSize = 2;


    // We will have just one position
    MatXd particle(1, 2);
    particle(0, 0) = 0.5 * double(iWidth);
    particle(0, 1) = 0.5 * double(iHeight);

    // Generating all images
    std::vector<MatXd> vImg(nImages);
    for (MatXd& mat : vImg)
    {
        mat = generateImage(iHeight, iWidth, signal, bGround, spotSize, particle);
        poissonNoise(mat);
    }

    // Creating a noisy particles
    std::random_device device;
    std::default_random_engine ran;
    std::normal_distribution<double> normal(0.0, 0.45);

    MatXd vec(nImages, 4);
    for (uint64_t k = 0; k < nImages; k++)
        vec.row(k) << double(k), double(k), 0.5 * double(iWidth) + normal(ran), 0.5 * double(iHeight) + normal(ran);


    // Enhancing trajectories
    GPT::Trajectory traj({ vImg });
    traj.spotSize = uint64_t(3.0*spotSize);

    traj.useRaw(std::vector<MatXd>{vec});
    traj.enhanceTracks();


    MatXd enhanced = traj.getTrack(0).traj[0];
    enhanced.col(GPT::Track::POSX).array() -= 0.5 * double(iWidth);
    enhanced.col(GPT::Track::POSY).array() -= 0.5 * double(iHeight);
    

    // Saving results for ploting
    Json::Value output;
    output["errorX"] = std::move(jsonEigen(VecXd(enhanced.col(GPT::Track::POSX))));
    output["errorY"] = std::move(jsonEigen(VecXd(enhanced.col(GPT::Track::POSY))));

    std::ofstream arq(outPath / "enhancement.json");
    arq << output;
    arq.close();

    // Running some basic tests
    double mu, dev, error;
    mu = enhanced.col(GPT::Track::POSX).mean(), 
    dev = sqrt((enhanced.col(GPT::Track::POSX).array() - mu).square().mean());
    error = enhanced.col(GPT::Track::ERRX).mean();
    EXPECT_LE(mu * sqrt(double(nImages)) / dev, 1.0) << "Enhancement :: Error in X is too great";
    EXPECT_GE(1.96 * error, dev) << " Enhancement :: Estimated error is to small";


    mu = enhanced.col(GPT::Track::POSY).mean();
    dev = sqrt((enhanced.col(GPT::Track::POSY).array() - mu).square().mean());
    error = enhanced.col(GPT::Track::ERRY).mean();
    EXPECT_LE(mu * sqrt(double(nImages)) / dev, 1.0) << "Enhancement :: Error in Y is too great";
    EXPECT_GE(1.96 * error, dev) << " Enhancement :: Estimated error is to small";



}
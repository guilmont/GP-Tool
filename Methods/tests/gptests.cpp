#include <gtest/gtest.h>
#include <fstream>

#include "GPMethods.h"


static Mat3d transformMatrix(double width, double height)
{
   double
        transX = 0.2367, transY = 2.2376,
        sclX = 1.0109, sclY = 1.0101,
        rotX = 255.2404, rotY = 255.0994,
        ang = -0.0186;

    Mat3d A, B, C, D;

    A << sclX, 0.0f, (1.0f - sclX) * 0.5f * width,
         0.0f, sclY, (1.0f - sclY) * 0.5f * height,
         0.0f, 0.0f, 1.0f;

    B << 1.0, 0.0, transX + rotX,
         0.0, 1.0, transY + rotY,
         0.0, 0.0, 1.0;

    C << cos(ang), -sin(ang), 0.0,
         sin(ang), cos(ang), 0.0,
        0.0, 0.0, 1.0;

    D << 1.0, 0.0, -rotX,
         0.0, 1.0, -rotY,
         0.0, 0.0, 1.0;

    return A * B * C * D;
}

struct Images : public testing::Test
{
    // We Are going to use this section to test the alignment algorithms and localization enhancement
    MatXd particles, rotatedPositions;
    std::vector<MatXd> vImages;

    const fs::path outPath = GTEST_PATH;


    void SetUp() override
    {
        // Let's start by generating a couple of frames with particles randomly placed
        const uint64_t
            width = 512,
            height = 512,
            nParticles = 100;

        const double
            signal = 150,
            bGround = 100,
            partSize = 3;

        std::random_device dev;
        std::default_random_engine ran(dev());
        std::uniform_real_distribution<double> unif(20.0, 492.0);

        // Generating random positions for particles
        // It might be better to test each new position so we don't have overlapping
        // Overlapping will potentially complicated some tests
        particles = MatXd(nParticles, 2);
        for (uint64_t k = 0; k < nParticles; k++)
        {
            uint64_t counter = 0;
            while (counter++ < 10*nParticles) // not ideal, but there are few positions
            {
                double x = unif(ran), y = unif(ran);
                bool good = true;
                for (uint64_t l = 0; l < k; l++)
                    if (abs(particles(l, 0) - x) < 5.0 * partSize && abs(particles(l, 1) - y) < 5.0 * partSize)
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


        // Generating channel 0
        MatXd mat = bGround * MatXd::Ones(height, width);
        for (uint32_t y = 0; y < height; y++)
            for (uint32_t x = 0; x < width; x++)
                for (uint64_t pt = 0; pt < nParticles; pt++)
                {
                    double
                        dx = (double(x) + 0.5 - particles(pt, 0)) / partSize,
                        dy = (double(y) + 0.5 - particles(pt, 1)) / partSize;

                    mat(y, x) += signal * exp(-0.5 * (dx * dx + dy * dy));
                }

        // Now, let's insert alignment problems 
        Mat3d trf = transformMatrix(double(width), double(height));
        Mat3d itrf = trf.inverse();

        rotatedPositions = MatXd(nParticles, 2);
        for (uint64_t k = 0; k < nParticles; k++)
        {
            rotatedPositions(k, 0) = trf(0, 0) * particles(k, 0) + trf(0, 1) * particles(k, 1) + trf(0, 2);
            rotatedPositions(k, 1) = trf(1, 0) * particles(k, 0) + trf(1, 1) * particles(k, 1) + trf(1, 2);
        }


        MatXd mat2 = bGround * MatXd::Ones(height, width);
        for (uint64_t k = 0; k < height; k++)
            for (uint64_t l = 0; l < width; l++)
            {
                int64_t col = int64_t(itrf(0, 0) * (double(l) + 0.5) + itrf(0, 1) * (double(k) + 0.5) + itrf(0, 2));
                int64_t row = int64_t(itrf(1, 0) * (double(l) + 0.5) + itrf(1, 1) * (double(k) + 0.5) + itrf(1, 2));

                if (row >= 0 && row < height && col >=0 && col < width)
                    mat2(k,l) = mat(row, col);
            }


        // Inserting poisson noise in images
        using param_t = std::poisson_distribution<uint64_t>::param_type;
        std::poisson_distribution<uint64_t> poisson(10.0);

        for (uint64_t k = 0; k < height; k++)
            for (uint64_t l = 0; l < width; l++)
            {
                poisson.param(param_t{ mat(k,l)});
                mat(k, l) = double(poisson(ran));

                poisson.param(param_t{ mat2(k,l) });
                mat2(k, l) = double(poisson(ran));
            }
   

        // Copying images into final vector for testing        
        vImages.emplace_back(mat);
        vImages.emplace_back(mat2);

    }
};


TEST_F(Images, alignment)
{
    // Let's calculated corrections
    GPT::Align align(1, vImages.data(), vImages.data()+1);
    ASSERT_EQ(true, align.alignCameras());
    ASSERT_EQ(true, align.correctAberrations());
    
    const GPT::TransformData& RT = align.getTransformData();
    
    // Let's realign image
    MatXd corrected = MatXd::Zero(vImages[0].rows(), vImages[0].cols());
    for (int64_t k = 0; k < corrected.rows(); k++)
        for (int64_t l = 0; l < corrected.cols(); l++)
        {
            int64_t x = int64_t(RT.itrf(0, 0) * (double(l) + 0.5) + RT.itrf(0, 1) * (double(k) + 0.5) + RT.itrf(0, 2));
            int64_t y = int64_t(RT.itrf(1, 0) * (double(l) + 0.5) + RT.itrf(1, 1) * (double(k) + 0.5) + RT.itrf(1, 2));

            if (y >= 0 && y < corrected.rows() && x >= 0 && x < corrected.cols())
                corrected(k, l) = vImages[1](y, x);
        }

    // Let's save some stuff for plotting later
    std::ofstream arq(outPath / "imageOriginal.txt");
    arq << vImages[0];
    arq.close();

    arq.open(outPath / "imageRotated.txt");
    arq << vImages[1];
    arq.close();

    arq.open(outPath / "imageCorrected.txt");
    arq << corrected;
    arq.close();

    arq.open(outPath / "positionOriginal.txt");
    arq << particles;
    arq.close();

    arq.open(outPath / "positionRotated.txt");
    arq << rotatedPositions;
    arq.close();


    // Let's check if the points will be realigned
    arq.open(outPath / "positionCorrected.txt");
    MatXd diff(particles.rows(), 2);
    for (int64_t k = 0; k < particles.rows(); k++)
    {
        double x = RT.trf(0, 0) * rotatedPositions(k, 0) + RT.trf(0, 1) * rotatedPositions(k, 1) + RT.trf(0, 2);
        double y = RT.trf(1, 0) * rotatedPositions(k, 0) + RT.trf(1, 1) * rotatedPositions(k, 1) + RT.trf(1, 2);

        arq << x << " " << y << std::endl;

        diff(k, 0) = sqrt(pow(x - particles(k, 0), 2.0) + pow(y - particles(k, 1), 2.0));
        diff(k, 1) = sqrt(pow(rotatedPositions(k,0) - particles(k, 0),2.0) + pow(rotatedPositions(k,1) - particles(k, 1), 2.0));
    }
    arq.close();

    double mu1 = diff.col(0).mean(), mu2 = diff.col(1).mean();

    EXPECT_LT(mu1, 0.5*mu2) << "Alignement :: Corrected position error is greater";

    arq.open(outPath / "distanceDistributions.txt");
    arq << diff;
    arq.close();
}

TEST_F(Images, enhancement)
{

}
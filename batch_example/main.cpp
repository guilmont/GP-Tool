#include <iostream>
#include <filesystem>
#include <thread>

// GP-Tool libraries
#include <movie.h>
#include <trajectory.h>
#include <gp_fbm.h>
#include <align.h>

void runSample(std::filesystem::path &path)
{
    std::string mov_name = path.string(),
                base = path.parent_path().string() + "/" + path.stem().string();

    std::vector<MatXd> vCH[2];
    std::array<std::string, 2> xml_name = {"_CH0.xml", "_CH1.xml"};

    Movie mov(mov_name);
    const Metadata &meta = mov.getMetadata();
    const uint32_t nChannels = meta.SizeC;

    ///////////////////////////////////////////////////////
    // Setting up trajectories
    Trajectory traj(&mov);
    traj.spotSize = 3; // selecting spot size

    for (uint32_t ch = 0; ch < nChannels; ch++)
    {
        // Getting a few frames for alignment
        for (uint32_t k = 0; k < 5; k++)
            vCH[ch].push_back(mov.getImage(ch, k));

        // Importing trajectories
        traj.useICY(base + xml_name[ch], ch);
    }
    // Enhancing tracks
    traj.enhanceTracks();

    ///////////////////////////////////////////////////////
    // Determining diffusion parameters
    const MatXd &mat0 = traj.getTrack(0).traj[0];
    const MatXd &mat1 = traj.getTrack(1).traj[0];

    if (mat0.rows() < 50 || mat1.rows() < 50)
    {
        std::cout << "WARN: Ignored due to short trajectory >> " << mov_name << std::endl;
        return;
    }

    GP_FBM gp(std::vector<MatXd>{mat0, mat1});
    GP_FBM::DA
        *da0 = gp.singleModel(0),
        *da1 = gp.singleModel(1);

    GP_FBM::CDA
        *cda = gp.coupledModel();

    if (cda == nullptr || da0 == nullptr || da1 == nullptr)
    {
        std::cout << "WARN: Ignore because didn't converge >> " << mov_name << std::endl;
        return;
    }

    ///////////////////////////////////////////////////////
    // Aligning channels
    Align align(uint32_t(vCH[0].size()), vCH[0].data(), vCH[1].data());

    if (!align.alignCameras())
        std::cout << "WARN: Couldn't align cameras >> " << mov_name << std::endl;

    if (!align.correctAberrations())
        std::cout << "WARN: Couldn't correct chromatic aberrations >> " << mov_name << std::endl;

    // Getting correction values
    const TransformData &trf = align.getTransformData();

    //////////////////////////////////////////////////////
    // Save results as necessary here

} // runSample

static void runParallel(uint32_t tid, char **argv, uint32_t argc, uint32_t nThreads)
{
    for (uint32_t l = tid + 1; l < argc; l += nThreads)
        runSample(std::filesystem::path(argv[l]));
}

int main(int argc, char *argv[])
{
    const uint32_t nThreads = 4;
    std::vector<std::thread> vThr(nThreads);

    for (uint32_t k = 0; k < nThreads; k++)
        vThr[k] = std::thread(runParallel, k, argv, uint32_t(argc), nThreads);

    for (std::thread &thr : vThr)
        thr.join();

    return EXIT_SUCCESS;
}

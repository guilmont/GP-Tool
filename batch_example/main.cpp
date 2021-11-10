// This program expects this file organization
// ProjectName
//     date0
//        movie1
//        movie2
//        movie3
//        ...
//     date1
//        another1
//        another2
//        another3
//        ...
// It will assume that each move has 1 spot per channel.


// It is going to check over all the input data for any problems.
//  - Were input and output directories properly set?
//  - Does files exist (Movie + tracks)? Can they be read? (Assuming ICY-XML files, but you can adapt)
//  - Do you have only one track per channel? (You can change this to you content)
//  - Are there trajectories with more than only spot per frame?
// If any of those tests fail, an output will be prompted and execution aborted after all tests are fineshed.

// If all the tests are fine, the program will import all the movies and tracks, enhance tracks and estimate D_A, alpha with and without substrate
// movement correction. It will also save a couple of frames from each movie in a day to perform alignment. All results will be saved in a GDManager
// file in the set output directory.

#include <GPMethods.h>
#include <GDManager.h>

constexpr uint64_t MIN_SIZE_TRAJECTORIES = 50; // Only trajectory longer than this will be considerer
constexpr uint64_t SPOT_SIZE = 3;              // Expected spot size, used in the enhancement method
constexpr uint64_t NUM_THREADS = 4;            // Split analysis in this many threads

struct MovData
{
    fs::path moviePath;
    std::vector<fs::path> trajPath; 
};

 static void runSample(const std::vector<MovData>& vecMovie, const fs::path &outpath);


int main()
{

    // Folder in which movies will be searched and save
    // Outfolder will be created if non-existent
    fs::path 
        mainPath = "Path to project here",
        outPath = "Output directory here";

    std::string nameFilter = "";   // Insert here any unique identifier for the tif files to be analyzed


    // Checking if directories are fine
    if (!fs::exists(mainPath))
    {
        GPT::pout("ERROR Input directory doesn't exist:", mainPath);
        return EXIT_FAILURE;
    }

    if (!fs::exists(outPath))
    {
        GPT::pout("ERROR Output directory doesn't exist:", outPath);
        return EXIT_FAILURE;
    }


    bool stopAnalysis = false;
    std::map<std::string, std::vector<MovData>> mov2Run;
    
    for (auto &entry : fs::directory_iterator(mainPath)) // loop over days
    {
        std::string date = entry.path().stem().string();
        for (auto const& sample : fs::recursive_directory_iterator(entry.path()))
         {
             // Is it of the type we are interested in?
             const fs::path& data = sample.path();
             if (fs::is_regular_file(sample.path()) && data.extension().string().compare(".tif") == 0 && data.string().find(nameFilter) != std::string::npos)
             {
                
                 bool trigger = false;
                 // Let's check if movie is good
                GPT::Movie mov(data);
                if (!mov.successful())
                {
                    GPT::pout("ERROR Cannot open movie:", data.string());
                    trigger = true;
                    continue;
                }


                MovData loc;
                loc.moviePath = data;

                int32_t numChannels = int32_t(movie.getMetadata().SizeC);

                GPT::Trajectory traj(&mov);
                for (int32_t ch = 0; ch < numChannels; ch++)
                {
                    fs::path trajPath = data.parent_path() / (data.stem().string() + "_CH" + std::to_string(ch) + ".xml");
                    loc.trajPath.push_back(trajPath);

                    if (!traj.useICY(trajPath, ch))
                    {
                        trigger = true;
                        continue;
                    }

                    if (traj.getTrack(ch).traj.size() != 1)
                    {
                        GPT::pout("ERROR ICY file contains multiple or no trajectories:", trajPath.string());
                        trigger = true;
                        continue;
                    }

                    // Checking if trajectories contain one data point per frame
                    const VecXd& vt = traj.getTrack(ch).traj[0].col(GPT::Track::FRAME);

                    // As it is already sorted, we can simply check next in line
                    for (int64_t k = 0; k < vt.size() - 1; k++)
                        if (vt(k+1) == vt(k))
                        {
                            GPT::pout("ERROR Trajectory contains multiple points in frame", vt(k), ": ", trajPath);
                            trigger = true;
                        }
                }

                if (!trigger)
                    mov2Run[date].emplace_back(std::move(loc));

                stopAnalysis |= trigger;
             }
         }
    }

    if (stopAnalysis)
    {
        GPT::pout("Aborting analysis...");
        return EXIT_FAILURE;
    }

    if (mov2Run.empty())
        GPT::pout("Nothing to run...");

    for (auto const& [day, vec] : mov2Run)
    {
        GPT::pout("Running", day, "...");
        fs::path gdmFile = outPath / (day + "_" + nameFilter + ".gdm");

        if (fs::exists(gdmFile))
            fs::remove(gdmFile);

        runSample(vec, gdmFile);
     }

    return EXIT_SUCCESS;
}


static void runSample(const std::vector<MovData>& vecMovie, const fs::path& outpath)
{
    // Preparing variables to hold alignment images
    const uint64_t
        numChannels = vecMovie[0].trajPath.size(),
        numSamples = vecMovie.size();

    std::vector<std::vector<MatXd>> vCH(numChannels, std::vector<MatXd>(vecMovie.size()));

    // Starting a fresh gdm file
    GDM::File gdm(outpath);
    GDM::Group& movGroup = gdm.addGroup("Movies");

    for (const MovData& info : vecMovie)
        movGroup.addGroup(info.moviePath.stem().string());


    auto parallel_function = [&](uint64_t threadID) -> void {
        for (uint64_t sid = threadID; sid < numSamples; sid += NUM_THREADS)
        {
            const MovData& info = vecMovie[sid];

            GPT::Movie mov(info.moviePath);
            const GPT::Metadata& meta = mov.getMetadata();
            double CONV = meta.PhysicalSizeXY * meta.PhysicalSizeXY;
            const uint64_t nChannels = meta.SizeC;

            // Setting up trajectories
            GPT::Trajectory traj(&mov);
            traj.spotSize = SPOT_SIZE;

            for (uint64_t ch = 0; ch < numChannels; ch++)
            {
                // Getting the first frame of each movie or sample alignment
                vCH[ch][sid] = mov.getImage(ch, 0);

                // Importing trajectories
                traj.useICY(info.trajPath[ch], ch);
            }

            // Enhancing tracks -> maybe in the furture this function could release RAM after done with each frame, what would make less succeptible to saturate RAM when using many threads
            traj.enhanceTracks();

            // Determining diffusion parameters
            const MatXd& mat0 = traj.getTrack(0).traj[0];
            const MatXd& mat1 = traj.getTrack(1).traj[0];

            if (mat0.rows() < MIN_SIZE_TRAJECTORIES || mat1.rows() < MIN_SIZE_TRAJECTORIES)
            {
                GPT::pout("WARN: Ignored due to short trajectory >>", info.moviePath.string());
                continue;
            }

            GPT::GP_FBM gp(std::vector<MatXd>{mat0, mat1});
            GPT::GP_FBM::DA
                * da0 = gp.singleModel(0),
                * da1 = gp.singleModel(1);

            GPT::GP_FBM::CDA* cda = gp.coupledModel();

            if (cda == nullptr || da0 == nullptr || da1 == nullptr)
            {
                GPT::pout("WARN: Ignore because didn't converge >>", info.moviePath.string());
                continue;
            }

            const MatXd& substrate = gp.estimateSubstrateMovement();

            //////////////////////////////
            // Saving all results

            GDM::Group& loc = movGroup.getGroup(info.moviePath.stem().string());

            // Saving trajectories
            GDM::Group& trajGroup = loc.addGroup("trajectories");
            trajGroup.add("trajectory_0", mat0.data(), { uint64_t(mat0.cols()), uint64_t(mat0.rows()) }); // MatXd (Eigen) is column major, GDM is row major
            trajGroup.add("trajectory_1", mat1.data(), { uint64_t(mat1.cols()), uint64_t(mat1.rows()) });

            // Saving some comments so we are not lost
            trajGroup.addDescription("physicalSizeXY", std::to_string(meta.PhysicalSizeXY));
            trajGroup.addDescription("physicalSizeXYUnit", meta.PhysicalSizeXYUnit);
            trajGroup.addDescription("timeIncrementUnit", meta.TimeIncrementUnit);
            trajGroup.addDescription("rows", "frame, time, pos_x, pos_y, error_x, error_y, size_x, size_y,background, signal");

            /////////////////////////////
            // Saving dynamics :: MatXd is Column major in GPTool

            MatXd aux(5, 2);
            aux.col(0) << 0, (CONV * da0->D), da0->A, da0->mu(0), da0->mu(1);
            aux.col(1) << 1, (CONV * da1->D), da1->A, da1->mu(0), da1->mu(1);

            GDM::Data& woutData = loc.addGroup("without_substrate").add("DA", aux.data(), { 2, 5 });
            woutData.addDescription("columns", "channel, D, A, mu_x, mu_y");
            woutData.addDescription("mu_units", "pixels");
            woutData.addDescription("D_units", meta.PhysicalSizeXYUnit + "^2/" + meta.TimeIncrementUnit + "^A");

            /////////////////////

            GDM::Group& withGroup = loc.addGroup("with_substrate");

            aux = MatXd(3, 2);
            aux.col(0) << 0, (CONV * cda->da[0].D), cda->da[0].A;
            aux.col(1) << 1, (CONV * cda->da[1].D), cda->da[1].A;

            GDM::Data& withData = withGroup.add("DA", aux.data(), { 2, 3 });
            withData.addDescription("columns", "channel, D, A");
            withData.addDescription("D_units", meta.PhysicalSizeXYUnit + "^2/" + meta.TimeIncrementUnit + "^A");


            double var[2] = { CONV * cda->DR, cda->AR };
            GDM::Group& subGroup = withGroup.addGroup("substrate");
            GDM::Data& subData = subGroup.add("DA", var, { 1,2 });
            subData.addDescription("columns", "DR, AR");
            subData.addDescription("D_units", meta.PhysicalSizeXYUnit + "^2/" + meta.TimeIncrementUnit + "^A");

            GDM::Data& data = subGroup.add("trajectory", substrate.data(), { uint64_t(substrate.cols()), uint64_t(substrate.rows()) });
            data.addDescription("rows", "frame, time, pos_x, pos_y, error_x, error_y");

        }
    };

    // Splitting load into many threads
    std::array<std::thread, NUM_THREADS> vthr;
    for (uint64_t tid = 0; tid < NUM_THREADS; tid++)
        vthr[tid] = std::thread(parallel_function, tid);

    for (std::thread& thr : vthr)
        thr.join();

    // Aligning channels
    GDM::Group& grp = gdm.addGroup("Alignment");
    for (uint64_t ch = 1; ch < numChannels; ch++)
    {
        GPT::Align align(vCH[0].size(), vCH[0].data(), vCH[ch].data());

        align.alignCameras();
        align.correctAberrations();

        const GPT::TransformData& RT = align.getTransformData();

        // Saving complete dataset
        GDM::Group& other = grp.addGroup("channel_" + std::to_string(ch));
        other.add("translate", RT.translate.data(), { 1, 2 });
        other.add("rotate", RT.rotate.data(), { 1, 3 });
        other.add("scale", RT.scale.data(), { 1, 2 });
        other.add("size", RT.size.data(), { 1, 2 });

        MatXd trans = RT.trf.transpose();
        other.add("transform", trans.data(), { 3, 3 });
    }

    // Wrinting to file
    gdm.save();
    gdm.close();

}

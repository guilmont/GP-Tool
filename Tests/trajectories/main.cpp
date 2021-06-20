#include "movie.h"
#include "trajectory.h"

int main()
{
    gpout(fs::current_path());

    const std::string runpath(OPATH);

    std::string xmlTracks[] = {runpath + "/test_data/test_localization_green.xml",
                               runpath + "/test_data/test_localization_red.xml"};

    Movie mov(runpath + "/test_data/test_localization.tif");

    Trajectory traj(&mov);
    traj.spotSize = 3;
    traj.useICY(xmlTracks[0], 0);
    traj.useICY(xmlTracks[1], 1);
    traj.enhanceTracks();

    const Metadata &meta = mov.getMetadata();

    gpout("Metadata:");
    gpout("acquisitionDate:", meta.acquisitionDate);
    gpout("SignificantBits:", meta.SignificantBits);
    gpout("DimensionOrder:", meta.DimensionOrder);
    gpout("SizeX:", meta.SizeX);
    gpout("SizeY:", meta.SizeY);
    gpout("SizeC:", meta.SizeC);
    gpout("SizeZ:", meta.SizeZ);
    gpout("SizeT:", meta.SizeT);
    gpout("PhysicalSizeXY:", meta.PhysicalSizeXY, meta.PhysicalSizeXYUnit);
    gpout("PhysicalSizeZ:", meta.PhysicalSizeZ, meta.PhysicalSizeZUnit);
    gpout("TimeIncrement:", meta.TimeIncrement, meta.TimeIncrementUnit);

    gpout("Channels:");
    for (auto var : meta.nameCH)
        gpout(" ::", var);

    gpout();

    const Track &green = traj.getTrack(0);
    std::ofstream arq(runpath + "/traj_green.txt");
    arq << green.traj[0];
    arq.close();

    const Track &red = traj.getTrack(1);
    arq.open(runpath + "/traj_red.txt");
    arq << red.traj[0];
    arq.close();

    return EXIT_SUCCESS;
}
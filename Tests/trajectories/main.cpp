#include "movie.h"
#include "trajectory.h"

int main()
{
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

    pout("Metadata:");
    pout("acquisitionDate:", meta.acquisitionDate);
    pout("SignificantBits:", meta.SignificantBits);
    pout("DimensionOrder:", meta.DimensionOrder);
    pout("SizeX:", meta.SizeX);
    pout("SizeY:", meta.SizeY);
    pout("SizeC:", meta.SizeC);
    pout("SizeZ:", meta.SizeZ);
    pout("SizeT:", meta.SizeT);
    pout("PhysicalSizeXY:", meta.PhysicalSizeXY, meta.PhysicalSizeXYUnit);
    pout("PhysicalSizeZ:", meta.PhysicalSizeZ, meta.PhysicalSizeZUnit);
    pout("TimeIncrement:", meta.TimeIncrement, meta.TimeIncrementUnit);

    pout("Channels:");
    for (auto var : meta.nameCH)
        pout(" ::", var);

    pout();

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
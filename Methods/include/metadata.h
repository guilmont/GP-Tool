#pragma once

#include "header.h"

#include "gtiffer.h"
#include "pugixml/pugixml.hpp"

namespace GPT
{

    struct Plane
    {
        uint64_t 
            TheC = 0,
            TheT = 0,
            TheZ = 0;

        float
            DeltaT = 0.0f,
            ExposureTime = 0.0f,
            PositionX = 0.0f,
            PositionY = 0.0f,
            PositionZ = 0.0f;

        std::string
            DeltaTUnit,
            ExposureTimeUnit,
            PositionXUnit,
            PositionYUnit,
            PositionZUnit;
    };

    class Metadata
    {
    public:
        uint64_t
            SignificantBits,
            SizeC, SizeT, SizeX, SizeY, SizeZ;

        double
            PhysicalSizeXY = 0.0,
            PhysicalSizeZ,
            TimeIncrement;

        std::string
            movie_name,
            acquisitionDate,
            DimensionOrder,
            PhysicalSizeXYUnit,
            PhysicalSizeZUnit,
            TimeIncrementUnit;

        std::string metaString;  // raw metadata in the format of a string
        std::vector<std::string> nameCH;

        GP_API Metadata(Tiffer::Read *tif);

        GP_API bool hasPlanes(void) const { return (vPlanes.size() > 0); }
        GP_API const Plane &getPlane(uint64_t c, uint64_t z, uint64_t t) const;

    private:
        std::vector<Plane> vPlanes;

        bool parseOME(const std::string &inputString);
        bool parseIJ(const std::string &inputString);
        bool parseIJ_extended(const std::string &inputString);
    };

}
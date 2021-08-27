#pragma once

#include "header.h"

#include "gtiffer.h"
#include "pugixml/pugixml.hpp"

namespace GPT
{

    struct Plane
    {
        uint32_t 
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
        uint32_t
            SignificantBits,
            SizeC, SizeT, SizeX, SizeY, SizeZ;

        float
            PhysicalSizeXY = 0.0f,
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
        GP_API const Plane &getPlane(uint32_t c, uint32_t z, uint32_t t) const;

    private:
        std::vector<Plane> vPlanes;

        bool parseOME(const std::string &inputString);
        bool parseIJ(const std::string &inputString);
        bool parseIJ_extended(const std::string &inputString);
    };

}
#pragma once

#include "header.h"

#include "gtiffer.h"
#include "pugixml/pugixml.hpp"

namespace GPT
{

    struct Plane
    {
        uint32_t TheC, TheT, TheZ;

        float
            DeltaT,
            ExposureTime,
            PositionX, PositionY, PositionZ;

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
            PhysicalSizeXY,
            PhysicalSizeZ,
            TimeIncrement;

        std::string
            movie_name,
            acquisitionDate,
            DimensionOrder,
            PhysicalSizeXYUnit,
            PhysicalSizeZUnit,
            TimeIncrementUnit;

        std::string metaString;
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
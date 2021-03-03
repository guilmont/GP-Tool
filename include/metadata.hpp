#pragma once
#include <config.hpp>
#include <tiffer.hpp>
#include <pugixml.hpp>

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

    Metadata(void) = default;
    Metadata(const std::string &movie, const std::string &value);

    bool parseOME(const std::string &inputString);
    bool parseImageJ(const std::string &inputString);

    bool hasPlanes(void) const { return (vPlanes.size() > 0); }

    const Plane &getPlane(uint32_t c, uint32_t z, uint32_t t) const;

private:
    std::vector<Plane> vPlanes;
    Mailbox *mail = nullptr;
};

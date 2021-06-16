#include "metadata.h"


Metadata::Metadata(Tiffer::Read *tif)
{
    // Determining movie name
    const std::string &movie_path = tif->getMoviePath();
    size_t pos = movie_path.find_last_of("/");
    movie_name = movie_path.substr(pos + 1);

    // Does it have extended IJ metadata
    std::string info = tif->getIJMetadata();
    if (info.size() > 0)
        if (parseIJ_extended(info))
            return;

    // Does it have OME metadata
    info = tif->getMetadata();
    if (info.find("OME") != std::string::npos)
        if (parseOME(info))
            return;

    // Does it have standard IJ metadata?
    if (info.find("ImageJ") != std::string::npos)
    {
        this->metaString = info;
        this->SignificantBits = tif->getBitCount();
        this->acquisitionDate = tif->getDateTime();
        this->SizeY = tif->getHeight();
        this->SizeX = tif->getWidth();
        this->PhysicalSizeXY = 1.0;
        this->PhysicalSizeZ = 1.0;
        this->TimeIncrement = 1.0;

        this->acquisitionDate = tif->getDateTime();
        this->DimensionOrder = "XYCZT";
        this->PhysicalSizeXYUnit = "Pixel";
        this->PhysicalSizeZUnit = "Pixel";
        this->TimeIncrementUnit = "Frame";

        if (parseIJ(info))
            return;
    }

    // Metadata structure is not known, we go with basic information
    this->SignificantBits = tif->getBitCount();
    this->SizeT = tif->getNumDirectories();
    this->SizeC = 1;
    this->SizeZ = 1;
    this->SizeY = tif->getHeight();
    this->SizeX = tif->getWidth();

    this->PhysicalSizeXY = 1.0;
    this->PhysicalSizeZ = 1.0;
    this->TimeIncrement = 1.0;

    this->acquisitionDate = tif->getDateTime();
    this->DimensionOrder = "XYCZT";
    this->PhysicalSizeXYUnit = "Pixel";
    this->PhysicalSizeZUnit = "Pixel";
    this->TimeIncrementUnit = "Frame";

    this->metaString = info;
    this->nameCH.emplace_back("channel0");

} // contructor

bool Metadata::parseOME(const std::string &inputString)
{
    pugi::xml_document doc;
    if (!doc.load_string(inputString.c_str()))
    {
        pout("ERROR (Metadata::parseOME) ==> Cannot parse OME metadata!! ::", movie_name);
        return false;
    }

    metaString = inputString; // hard copy for display in gui

    pugi::xml_node ome = doc.child("OME");
    pugi::xml_node main = ome.child("Image");
    pugi::xml_node Pixels = main.child("Pixels");

    // Loading image info
    acquisitionDate = main.child_value("AcquisitionDate");

    // Loading pixel info
    SizeC = Pixels.attribute("SizeC").as_uint();
    SizeT = Pixels.attribute("SizeT").as_uint();
    SizeX = Pixels.attribute("SizeX").as_uint();
    SizeY = Pixels.attribute("SizeY").as_uint();
    SizeZ = Pixels.attribute("SizeZ").as_uint();
    SignificantBits = Pixels.attribute("SignificantBits").as_uint();

    PhysicalSizeXY = Pixels.attribute("PhysicalSizeX").as_float();
    PhysicalSizeZ = Pixels.attribute("PhysicalSizeZ").as_float();
    TimeIncrement = Pixels.attribute("TimeIncrement").as_float();

    DimensionOrder = Pixels.attribute("DimensionOrder").as_string();
    PhysicalSizeXYUnit = Pixels.attribute("PhysicalSizeXUnit").as_string();
    PhysicalSizeZUnit = Pixels.attribute("PhysicalSizeZUnit").as_string();
    TimeIncrementUnit = Pixels.attribute("TimeIncrementUnit").as_string();

    // Loafing channels's name
    for (auto &ch : Pixels.children("Channel"))
        nameCH.push_back(ch.attribute("Name").as_string());

    // Loading planes
    for (auto &pl : Pixels.children("Plane"))
    {
        Plane pne;
        pne.TheC = pl.attribute("TheC").as_uint();
        pne.TheT = pl.attribute("TheT").as_uint();
        pne.TheZ = pl.attribute("TheZ").as_uint();

        pne.DeltaT = pl.attribute("DeltaT").as_float();
        pne.ExposureTime = pl.attribute("ExposureTime").as_float();
        pne.PositionX = pl.attribute("PositionX").as_float();
        pne.PositionY = pl.attribute("PositionY").as_float();
        pne.PositionZ = pl.attribute("PositionZ").as_float();

        pne.DeltaTUnit = pl.attribute("DeltaTUnit").as_string();
        pne.ExposureTimeUnit = pl.attribute("ExposureTimeUnit").as_string();
        pne.PositionXUnit = pl.attribute("PositionXUnit").as_string();
        pne.PositionYUnit = pl.attribute("PositionYUnit").as_string();
        pne.PositionZUnit = pl.attribute("PositionZUnit").as_string();

        vPlanes.emplace_back(pne);
    } // loop-planes

    return true;
} // parseOME

bool Metadata::parseIJ(const std::string &inputString)
{
    metaString = inputString;

    auto getInfo = [&](const std::string &tag) -> std::string {
        size_t beg = inputString.find(tag);
        if (beg == std::string::npos)
            return "";

        beg += tag.size();
        size_t end = inputString.find('\n', beg);

        return inputString.substr(beg, end - beg);
    }; // getInfo

    auto help = [](const std::string &value) -> uint32_t {
        if (value.size() == 0)
            return 1;
        else
            return stoi(value);
    };

    this->SizeT = help(getInfo("frames="));
    this->SizeC = help(getInfo("channels="));
    this->SizeZ = help(getInfo("slices="));

    for (uint32_t ch = 0; ch < SizeC; ch++)
        this->nameCH.emplace_back("channel" + std::to_string(ch));

    return true;
}

bool Metadata::parseIJ_extended(const std::string &inputString)
{
    metaString = inputString; // hard copy for display in the gui later

    auto getInfo = [&](const std::string &tag) -> std::string {
        size_t beg = inputString.find(tag);
        if (beg == std::string::npos)
            return "";

        beg += tag.size();
        size_t end = inputString.find('\n', beg);

        return inputString.substr(beg, end - beg);
    }; // getInfo

    // To fix some IJ blind OME copies
    std::string help = getInfo("BitsPerPixel = ");
    if (help.size() == 0)
        return false;

    SignificantBits = stoi(help);
    SizeC = stoi(getInfo("SizeC = "));
    SizeT = stoi(getInfo("SizeT = "));
    SizeX = stoi(getInfo("SizeX = "));
    SizeY = stoi(getInfo("SizeY = "));
    SizeZ = stoi(getInfo("SizeZ = "));
    DimensionOrder = getInfo("DimensionOrder = ");

    if (getInfo("spatial-calibration-state").compare("on") == 0)
    {
        std::string txt = getInfo("spatial-calibration-x = ");
        PhysicalSizeXY = stof(txt.size() == 0 ? "1" : txt);

        txt = getInfo("spacing = ");
        PhysicalSizeZ = stof(txt.size() == 0 ? "1" : txt);
        PhysicalSizeXYUnit = getInfo("spatial-calibration-units = ");
        PhysicalSizeZUnit = getInfo("spatial-calibration-units = ");
    }
    else
    {
        PhysicalSizeXY = 1.0;
        PhysicalSizeZ = 1.0;
        PhysicalSizeXYUnit = "Pixel";
        PhysicalSizeZUnit = "Pixel";
    }

    acquisitionDate = getInfo("DateTime = ");

    std::string txt = getInfo("finterval = ");
    TimeIncrement = stof(txt.size() == 0 ? "1" : txt);

    TimeIncrementUnit = "secs";

    for (uint32_t ch = 0; ch < SizeC; ch++)
        nameCH.emplace_back(getInfo("WaveName" + std::to_string(ch + 1) + " = "));

    return true;
} // parseImageJ

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const Plane &Metadata::getPlane(uint32_t c, uint32_t z, uint32_t t) const
{
    for (const Plane &pl : vPlanes)
    {
        bool check = true;
        check &= pl.TheC == c;
        check &= pl.TheZ == z;
        check &= pl.TheT == t;

        if (check)
            return pl;
    }

    pout("WARN (Metadata::getPlane) => No plane was found for (c,z,t): ", c, z, t);
    return vPlanes.front();

} // getPlane
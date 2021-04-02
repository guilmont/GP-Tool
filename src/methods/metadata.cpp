#include <metadata.hpp>
#include <string>

bool Metadata::parseOME(const std::string &inputString)
{
    metaString = inputString; // hard copy for display in gui

    pugi::xml_document doc;
    if (!doc.load_string(inputString.c_str()))
    {
        mail->createMessage<MSG_Warning>("(Metadata::parseOME): "
                                         "Cannot parse OME metadata!!");
        return false;
    }

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

bool Metadata::parseImageJ(const std::string &inputString)
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

    SignificantBits = stoi(getInfo("BitsPerPixel = "));
    SizeC = stoi(getInfo("SizeC = "));
    SizeT = stoi(getInfo("SizeT = "));
    SizeX = stoi(getInfo("SizeX = "));
    SizeY = stoi(getInfo("SizeY = "));
    SizeZ = stoi(getInfo("SizeZ = "));
    DimensionOrder = getInfo("DimensionOrder = ");

    if (getInfo("spatial-calibration-state").compare("on") == 0)
    {
        PhysicalSizeXY = stof(getInfo("spatial-calibration-x = "));
        PhysicalSizeZ = stof(getInfo("spacing="));
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
    TimeIncrement = stof(getInfo("finterval="));

    TimeIncrementUnit = "secs";

    for (uint32_t ch = 0; ch < SizeC; ch++)
        nameCH.emplace_back(getInfo("WaveName" + std::to_string(ch + 1) + " = "));

    return true;
} // parseImageJ

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

    String txt = "(Metadata::getPlane): No plane was found for (c,z,t): " +
                 std::to_string(c) + ", " + std::to_string(z) + ", " + std::to_string(t);

    mail->createMessage<MSG_Error>(txt);

    return vPlanes.back();

} // getPlane
#include "imageProperties.h"

ImgProps::ImgProps(void)
{
    vec.emplace_back("None");
    vec.emplace_back("Red");
    vec.emplace_back("Green");
    vec.emplace_back("Blue");
    vec.emplace_back("White");
    vec.emplace_back("Cyan");
    vec.emplace_back("Orange");

    data["None"] = {0.0f, 0.0f, 0.0f};
    data["Red"] = {1.0f, 0.0f, 0.0f};
    data["Green"] = {0.0f, 1.0f, 0.0f};
    data["Blue"] = {0.0f, 0.0f, 1.0f};
    data["White"] = {1.0f, 1.0f, 1.0f};
    data["Cyan"] = {0.0f, 0.5f, 1.0f};
    data["Orange"] = {1.0f, 0.5f, 0.0f};

} // constructor

void ImgProps::setProps(uint32_t ch)
{
    channel[ch].lut = vec[ch + 1];
    channel[ch].contrast = {0.0f, 1.0f};
    channel[ch].minMaxValue = {0.0f, 1.0f};
    channel[ch].histogram = {0.0f};
}

void ImgProps::setLUT(uint32_t ch, const String &lut_name)
{
    channel[ch].lut = lut_name;
}

void ImgProps::setConstrast(uint32_t ch, float low, float high)
{
    channel[ch].contrast = {low, high};
}

void ImgProps::setMinMaxValues(uint32_t ch, float minValue, float maxValue)
{
    channel[ch].minMaxValue = {minValue, maxValue};
}
#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <array>

using String = std::string;
using LUT = std::array<float, 3>;
using Contrast = std::pair<float, float>;
using Histogram = std::array<float, 256>;

class ImgProps
{
public:
    ImgProps(void);

    void setProps(uint32_t ch);
    void setLUT(uint32_t ch, const String &lut_name);
    void setConstrast(uint32_t ch, float low, float high);
    void setMinMaxValues(uint32_t ch, float minValue, float maxValue);

    const std::vector<String> &getList() const { return vec; }
    const LUT &getLUT(uint32_t ch) { return data[channel[ch].lut]; }
    const String &getName(uint32_t ch) { return channel[ch].lut; }
    const Contrast &getContrast(uint32_t ch) { return channel[ch].contrast; }
    const Contrast &getMinMaxValues(uint32_t ch) { return channel[ch].minMaxValue; }

    Histogram &getHistogram(uint32_t ch) { return channel[ch].histogram; }

private:
    struct Info
    {
        String lut;
        Contrast contrast, minMaxValue;
        Histogram histogram;
    };

    std::vector<String> vec;
    std::unordered_map<String, LUT> data;
    std::unordered_map<uint32_t, Info> channel;
};

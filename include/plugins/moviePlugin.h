#pragma once

#include "plugin.h"
#include "methods/movie.h"
#include "gptool.h"

#include <vector>
#include <unordered_map>

class LUT
{
public:
    LUT(void);

    std::vector<std::string> names;
    const glm::vec3 &getColor(const std::string &name) const;

private:
    std::vector<glm::vec3> colors;
};

///////////////////////////////////////////////////////////

class MoviePlugin : public Plugin
{
public:
    MoviePlugin(const std::string &path, GPTool *ptr);
    ~MoviePlugin(void);

    int32_t current_frame = 0;

    void showProperties(void) override;
    void update(float deltaTime) override;

    bool successful(void) const { return success; }

    void setLUT(uint32_t ch, const std::string &lut_name);
    void setConstrast(uint32_t ch, const glm::vec2 &contrast);
    void setMinMaxValues(uint32_t ch, const glm::vec2 &values);

    // const Contrast &getContrast(uint32_t ch) { return channel[ch].contrast; }
    // const Contrast &getMinMaxValues(uint32_t ch) { return channel[ch].minMaxValue; }
    // Histogram &getHistogram(uint32_t ch) { return channel[ch].histogram; }

private:
    struct Info
    {
        std::string lut_name;
        glm::vec2 contrast, minMaxValue;
        std::array<float, 256> histogram;
    };

    GPTool *tool = nullptr;
    bool success = true;

    LUT lut;
    std::vector<Info> data;

    Movie movie;
};

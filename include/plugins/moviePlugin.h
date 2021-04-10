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

    const glm::vec3 &getColor(uint32_t channel) { return lut.getColor(info[channel].lut_name); }
    const Movie *getMovie(void) { return &movie; }
    bool successful(void) const { return success; }

private:
    void calcHistogram(uint32_t channel);
    void updateTexture(uint32_t channel);

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
    std::vector<Info> info;
    std::vector<std::unique_ptr<Framebuffer>> histo;
    std::unique_ptr<Texture> texture = nullptr;

    Movie movie;
};

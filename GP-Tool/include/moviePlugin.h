#pragma once

#include "gpch.h"
#include "plugin.h"
#include "movie.h"

#include "gptool.h"
class GPTool;

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
    MoviePlugin(const fs::path &path, GPTool *ptr);
    ~MoviePlugin(void);

    uint64_t current_frame = 0;

    void showProperties(void) override;
    void update(float deltaTime) override;
    bool saveJSON(Json::Value &json) override;

    const glm::vec3 &getColor(uint64_t channel) { return lut.getColor(info[channel].lut_name); }
    
    GPT::Movie *getMovie(void) { return movie.get(); }
    bool successful(void) const { return success; }

private:
    void calcHistogram(uint64_t channel);
    void updateTexture(uint64_t channel);
    
    std::unique_ptr<GRender::Quad> quad = nullptr;


private:
    struct Info
    {
        std::string lut_name;
        glm::vec2 contrast, minMaxValue;
        std::array<float, 256> histogram;
    };



    LUT lut;
    std::vector<Info> info;
    std::vector<std::unique_ptr<GRender::Framebuffer>> histo;

    GPTool *tool = nullptr;
    std::unique_ptr<GPT::Movie> movie = nullptr;

    bool success = true;
};

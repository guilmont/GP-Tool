#pragma once

// gp_fbm_library
#include "movie.hpp"
#include "align.hpp"
#include "trajectory.hpp"
#include "gp_fbm.hpp"
#include "imageProperties.h"

#include <unordered_map>

struct Plugin
{
    bool active = false;
    void (*callback)(void *);
};

struct Roi
{
    ImVec4 cor{39 / 255.0f, 174 / 255.0f, 96 / 255.0f, 0.1};
    std::vector<float> points;
};

struct MainStruct
{
    std::mutex mtx;
    uint32_t frame = 0;

    //////////////////////
    // plugins

    String activePlugin;
    std::unordered_map<String, Plugin> mPlugin;

    //////////////////////
    // viewport handling

    uint32_t texID_signal;

    bool viewHover = false;
    glm::vec2 viewPos, imgPos;
    glm::ivec2 viewport = {500, 500};
    ImgProps iprops;

    bool updateContrast_flag = false,
         updateViewport_flag = true,
         createBuffers_flag = false,
         openMovie_flag = false;

    //////////////////////
    // User data
    std::unique_ptr<Movie> movie = nullptr;
    std::unique_ptr<Align> align = nullptr;
    std::unique_ptr<Trajectory> track = nullptr;
    std::vector<GP_FBM> vecGP; // we might have several cells per movie

    std::unique_ptr<Roi> roi = nullptr;

    ////////////////////

    void resetData(const String &plugin)
    {
        // I have pointer of this class everywhere, not sure I could simply construct a new one
        activePlugin = std::move(plugin);
        movie = nullptr;
        align = nullptr;
        track = nullptr;
        vecGP.clear();
    }
};

// Some nice colors for the colormap
namespace g_color
{
    const ImVec4
        black(0.000f, 0.0000f, 0.0000f, 1.0f),
        gray(0.5490f, 0.5490f, 0.5490f, 1.0f),
        white(1.000f, 1.0000f, 1.0000f, 1.0f),
        red(0.7529f, 0.2235f, 0.1686f, 1.0f),
        blue(0.0784f, 0.2549f, 0.3882f, 1.0f),
        green(0.0980f, 0.4353f, 0.2392f, 1.0f),
        purple(0.5569f, 0.2667f, 0.6784f, 1.0f),
        orange(0.9020f, 0.4941f, 0.1333f, 1.0f),
        yellow(0.9765f, 1.0f, 0.2314f, 1.0f);
} // namespace g_color

#pragma once

#include "gpch.h"

#include "moviePlugin.h"
#include "alignPlugin.h"
#include "trajPlugin.h"
#include "gpPlugin.h"

#include "batch.h"

class GPTool : public GRender::Application
{
public:
    GPTool(void);
    ~GPTool(void);

    // Plugin related functions
    void showHeader(void);
    void showWindows(void);
    void showProperties(void);
    void updateAll(float deltaTime);

    void addPlugin(const std::string &name, Plugin *plugin);
    Plugin *getPlugin(const std::string &name);
    void setActive(const std::string &name);

    // Main loop functions
    void onUserUpdate(float deltaTime) override;
    void ImGuiLayer(void) override;
    void ImGuiMenuLayer(void) override;

    void openMovie(const fs::path &path);
    void saveJSON(const fs::path &path);

public:
    GRender::Texture texture;
    GRender::Shader shader;
    GRender::Camera2D camera;

    std::unique_ptr<GRender::Framebuffer> viewBuf = nullptr; // Histograms and viewport
    
    Batching batch;

private:
    // Layout used for first time ever
    glm::vec2 plPos = {0.01f, 0.05f}, plSize = {0.37f, 0.28f};
    glm::vec2 propPos = {0.01f, 0.34f}, propSize = {0.37f, 0.65f};
    glm::vec2 vwPos = {0.39f, 0.05f}, vwSize = {0.6f, 0.94f};

private:
    Plugin *pActive = nullptr;
    std::map<std::string, std::unique_ptr<Plugin>> plugins;

    // flow variables
    bool viewport_hover = false;

    glm::vec2 getClickPosition(void);

};

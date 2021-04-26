#pragma once

#include "renderer/renderer.h"

class PluginManager;
#include "plugins/manager.h"

class GPTool : public Renderer
{
public:
    GPTool(void);

    void onUserUpdate(float deltaTime) override;
    void ImGuiLayer(void) override;
    void ImGuiMenuLayer(void) override;

    void openMovie(const String &path);

    std::unique_ptr<PluginManager> manager = nullptr;

    std::unique_ptr<Quad> quad = nullptr;
    std::unique_ptr<Shader> shader = nullptr;
    std::unique_ptr<Framebuffer> viewBuf = nullptr; // Histograms and viewport

    // flow variables
    bool viewport_hover = false;
};
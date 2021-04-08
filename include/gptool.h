#pragma once

#include "config.h"
#include "renderer.h"

class PluginManager;
#include "plugins/manager.h"

class GPTool : public Renderer
{
public:
    GPTool(void);

    void onUserUpdate(float deltaTime) override;
    void ImGuiLayer(void) override;
    void ImGuiMenuLayer(void) override;

    void viewport_function(float deltaTime);

    void openMovie(const String &path);

private:
    std::unique_ptr<PluginManager> manager = nullptr;

    std::unique_ptr<Quad> quad = nullptr;
    std::unique_ptr<Shader> shader = nullptr;
    std::unordered_map<String, std::unique_ptr<Framebuffer>> fBuffer; // Histograms and viewport

    // flow variables
    bool viewport_hover = false;
};
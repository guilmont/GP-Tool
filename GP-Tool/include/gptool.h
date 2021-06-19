#pragma once

#include "gpch.h"

#include "mailbox.h"
#include "moviePlugin.h"
#include "alignPlugin.h"
#include "trajPlugin.h"
#include "gpPlugin.h"

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

    Mailbox mbox;
    std::unique_ptr<GRender::Quad> quad = nullptr;
    std::unique_ptr<GRender::Framebuffer> viewBuf = nullptr; // Histograms and viewport

private:
    Plugin *pActive = nullptr;
    std::map<std::string, std::unique_ptr<Plugin>> plugins;

    // flow variables
    bool viewport_hover = false;

private:
    void openMovie(const std::string &path);
    void saveJSON(const std::string &path);
};

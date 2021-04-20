#pragma once

#include <string>
#include <map>

#include "plugin.h"
#include "moviePlugin.h"
#include "alignPlugin.h"
#include "trajPlugin.h"

#include "renderer/fonts.h"

/////////////////////////////

class PluginManager
{
public:
    PluginManager(Fonts *fonts);
    ~PluginManager(void);

    void showHeader(void);
    void showWindows(void);
    void showProperties(void);
    void updateAll(float deltaTime);

    void addPlugin(const std::string &name, Plugin *plugin) { plugins[name].reset(plugin); }
    void setActive(const std::string &name) { pActive = plugins[name].get(); }

    Plugin *getPlugin(const std::string &name);

private:
    Fonts *fonts = nullptr;
    Plugin *pActive = nullptr;
    std::map<std::string, std::unique_ptr<Plugin>> plugins;
};
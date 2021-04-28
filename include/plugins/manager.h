#pragma once

#include <string>
#include <map>

#include "plugin.h"
#include "moviePlugin.h"
#include "alignPlugin.h"
#include "trajPlugin.h"
#include "gpPlugin.h"

#include "gptool.h"
#include "json/json.h"

/////////////////////////////

class PluginManager
{
public:
    PluginManager(GPTool *ptr);
    ~PluginManager(void);

    void showHeader(void);
    void showWindows(void);
    void showProperties(void);
    void updateAll(float deltaTime);

    void addPlugin(const std::string &name, Plugin *plugin) { plugins[name].reset(plugin); }
    void setActive(const std::string &name) { pActive = plugins[name].get(); }

    Plugin *getPlugin(const std::string &name);

    void saveJSON(const std::string &path);

private:
    GPTool *tool = nullptr;
    Plugin *pActive = nullptr;
    std::map<std::string, std::unique_ptr<Plugin>> plugins;
};

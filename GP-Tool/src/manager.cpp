#include "manager.h"


void PluginManager::showHeader(void)
{
   

} // showHeader

void PluginManager::updateAll(float deltaTime)
{
    for (auto &[name, ptr] : plugins)
        if (ptr)
            ptr->update(deltaTime);

} // update

Plugin *PluginManager::getPlugin(const std::string &name)
{
    if (plugins[name])
        return plugins[name].get();
    else
        return nullptr;
} //

void PluginManager::saveJSON(const std::string &path)
{

   /* Json::Value output;

    Plugin *pgl = plugins["MOVIE"].get();
    if (pgl)
        pgl->saveJSON(output);
    else
    {
        tool->mbox.create<Message::Info>("Nothing to be save!!");
        return;
    }

    pgl = plugins["ALIGNMENT"].get();
    if (pgl)
        pgl->saveJSON(output["Alignment"]);

    pgl = plugins["TRAJECTORY"].get();
    if (pgl)
    {
        Json::Value aux;
        if (pgl->saveJSON(aux))
            output["Trajectory"] = std::move(aux);
    }

    pgl = plugins["GPROCESS"].get();
    if (pgl)
    {
        Json::Value aux;
        if (pgl->saveJSON(aux))
            output["GProcess"] = std::move(aux);
    }

    std::ofstream arq(path);
    arq << output;
    arq.close();

    tool->mbox.create<Message::Info>("File saved to '" + path + "'");*/

} // saveJSON
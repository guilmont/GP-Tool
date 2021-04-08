#include "manager.h"

#include <imgui.h>
#include <iostream>

PluginManager::PluginManager(Fonts *fonts) : fonts(fonts)
{
    plugins["ALIGNMENT"] = nullptr;
    plugins["GPROCESS"] = nullptr;
    plugins["MOVIE"] = nullptr;
    plugins["TRAJECTORIES"] = nullptr;
}

PluginManager::~PluginManager(void) {}

void PluginManager::showHeader(void)
{
    ImGui::Begin("Plugins");

    float width = ImGui::GetContentRegionAvailWidth();
    ImVec2 buttonSize{width, 40};

    ImVec4
        deactivated{0.13f, 0.16f, 0.3f, 1.f},
        chosenColor{0.1f, 0.6f, 0.1f, 1.0f},
        hoverColor{0.2f, 0.7f, 0.2f, 1.0f};

    fonts->push("bold");

    for (auto const &[name, ptr] : plugins)
    {
        bool check = (pActive == ptr.get()) && ptr;

        if (check)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, chosenColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
        }
        else if (!ptr)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, deactivated);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, deactivated);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, deactivated);
        }

        if (ImGui::Button(name.c_str(), buttonSize) && ptr)
            pActive = ptr.get();

        if (check || !ptr)
            ImGui::PopStyleColor(3);
    }

    fonts->pop();

    ImGui::End();

} // showHeader

void PluginManager::showProperties(void)
{
    if (pActive)
        pActive->showProperties();
} // showProperties

void PluginManager::updateAll(float deltaTime)
{
    for (auto &[name, ptr] : plugins)
        if (ptr)
            ptr->update(deltaTime);

} // update
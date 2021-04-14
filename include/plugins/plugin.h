#pragma once

#include <imgui.h>
#include <glm/glm.hpp>

class GPTool;

struct Plugin
{
    Plugin(void) = default;
    virtual ~Plugin(void) = default;

    virtual void update(float deltaTime) = 0;
    virtual void showProperties(void) = 0;
};
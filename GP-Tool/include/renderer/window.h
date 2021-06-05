#pragma once

#include <imgui.h>

class Window
{
public:
    Window(void) = default;
    virtual ~Window(void) = default;

    bool active = false;
    void show(void) { active = true; }
    void hide(void) { active = false; }

    virtual void display(void) = 0;
    void setUserData(void *ptr) { user_data = ptr; }

protected:
    void *user_data = nullptr;
};

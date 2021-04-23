#pragma once

// vendor
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <implot.h>

// local
#include "config.h"
#include "quad.h"
#include "shader.h"
#include "framebuffer.h"
#include "texture.h"
#include "events.h"
#include "gdialog.h"
#include "camera.h"
#include "fonts.h"

#include "utils/mailbox.h"

struct Window
{
    String title;
    glm::vec2 position, size;
    GLFWwindow *ptr = nullptr;
};

class Renderer
{
public:
    Renderer(void) = default;
    virtual ~Renderer(void);

    void initialize(const String &name, uint32_t width, uint32_t height);
    void closeApp(void) { glfwSetWindowShouldClose(window.ptr, 1); }
    void mainLoop(void);

    virtual void onUserUpdate(float deltaT) = 0;
    virtual void ImGuiLayer(void) {}
    virtual void ImGuiMenuLayer(void) {}

    Window window;
    Mailbox mbox;
    GDialog dialog;

    Fonts fonts;

    Mouse mouse;
    Camera camera;
    Keyboard keyboard;

private:
    float deltaTime = 0.1f;
    friend void winResize_callback(GLFWwindow *, int, int);
    friend void winPos_callback(GLFWwindow *, int, int);
    friend void mousePos_callback(GLFWwindow *, double, double);
    friend void mouseScroll_callback(GLFWwindow *, double, double);
    friend void mouseButton_callback(GLFWwindow *, int, int, int);
    friend void keyboard_callback(GLFWwindow *, int, int, int, int);
};

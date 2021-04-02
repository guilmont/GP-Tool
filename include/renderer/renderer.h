#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <unordered_map>

// vendor
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// local
#include "config.h"
#include "quad.h"
#include "shader.h"
#include "framebuffer.h"
#include "events.h"
#include "gdialog.h"

class Renderer
{
public:
    Renderer(void) = default;
    virtual ~Renderer(void);

    void initialize(const String &name, uint32_t width, uint32_t height);
    void closeApp(void) { glfwSetWindowShouldClose(main_window, 1); }
    void mainLoop(void);

    virtual void onUserUpdate(float deltaT) = 0;
    virtual void ImGuiLayer(void) {}
    virtual void ImGuiMenuLayer(void) {}

    GDialog dialog;

    std::unique_ptr<Mouse> mouse = nullptr;
    std::unique_ptr<Keyboard> keyboard = nullptr;

private:
    String screenTitle;
    glm::ivec2 screenDim;

    float deltaTime = 0.1f;

    GLFWwindow *main_window = nullptr;

    friend void winResize_callback(GLFWwindow *, int, int);
    friend void mousePos_callback(GLFWwindow *, double, double);
    friend void mouseScroll_callback(GLFWwindow *, double, double);
    friend void mouseButton_callback(GLFWwindow *, int, int, int);
    friend void keyboard_callback(GLFWwindow *, int, int, int, int);
};

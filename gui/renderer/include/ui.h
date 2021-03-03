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

#include <string>
#include <map>

// local
#include "mailbox.hpp"

#include "camera.h"
#include "shader.h"
#include "texture.h"
#include "framebuffer.h"
#include "quad.h"
#include "fonts.h"

#include "window.h"
#include "fileDialog.h"
#include "winInboxMessage.hpp"

using String = std::string;

struct Mouse
{
    int action;
    bool leftButton = false, rightButton = false, middleButton = false;

    bool leftPressed = false, rightPressed = false, middlePressed = false;

    glm::vec2 offset;
    glm::vec2 pos = {0.0f, 0.0f};
    glm::vec2 dpos = {0.0f, 0.0f};
};

class UI
{
public:
    UI(const String &name, uint32_t width, uint32_t height);
    ~UI(void);

    void setUserData(void *ptr) { user_data = ptr; }
    void *getUserData(void) { return user_data; }

    void mainLoop(void (*onUserUpdate)(UI &),
                  void (*controls)(UI &),
                  void (*ImGuiMenuLayer)(UI &),
                  void (*ImGuiLayer)(UI &));

    void closeApp(void) { glfwSetWindowShouldClose(main_window, 1); }

    /////////////////////////
    // Rendered control

    Mailbox mail;
    std::map<String, std::unique_ptr<Window>> mWindows;

    Camera cam;
    Shader shader;
    Texture tex;
    Fonts fonts;
    Quad quad;
    std::map<String, std::unique_ptr<Framebuffer>> FBuffer;

    Mouse mouse;
    bool resizeEvent = false;
    float elapsedTime = 0.1f;

private:
    String screenTitle;
    glm::ivec2 screenDim;

    GLFWwindow *main_window = nullptr;
    void *user_data = nullptr;

    friend void winResize_callback(GLFWwindow *, int, int);
    friend void mousePos_callback(GLFWwindow *, double, double);
    friend void mouseScroll_callback(GLFWwindow *, double, double);
    friend void mouseButton_callback(GLFWwindow *, int, int, int);
};

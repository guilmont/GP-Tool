#include "ui.h"

#include "gl_assert.cpp"

///////////////////////////////////////////////////////////////////////////////

void winResize_callback(GLFWwindow *window, int width, int height)
{
    UI &ui = *reinterpret_cast<UI *>(glfwGetWindowUserPointer(window));

    ui.screenDim = {width, height};
    gl_call(glad_glViewport(0, 0, width, height));
    ui.resizeEvent = true;

} //winResize_callback

void mousePos_callback(GLFWwindow *window, double xpos, double ypos)
{
    UI &ui = *reinterpret_cast<UI *>(glfwGetWindowUserPointer(window));

    ui.mouse.dpos = {xpos - ui.mouse.pos.x, ypos - ui.mouse.pos.y};
    ui.mouse.pos = {xpos, ypos};

} //mousePos_callback

void mouseButton_callback(GLFWwindow *window, int button, int action, int mods)
{
    UI &ui = *reinterpret_cast<UI *>(glfwGetWindowUserPointer(window));

    (void)mods; // to avoid warnings

    ui.mouse.action = action;
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        ui.mouse.leftButton = true;
        if (action == GLFW_PRESS)
            ui.mouse.leftPressed = true;
        else if (action == GLFW_RELEASE)
            ui.mouse.leftPressed = false;
    }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        ui.mouse.rightButton = true;
        if (action == GLFW_PRESS)
            ui.mouse.rightPressed = true;
        else if (action == GLFW_RELEASE)
            ui.mouse.rightPressed = false;
    }

    else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        ui.mouse.middleButton = true;
        if (action == GLFW_PRESS)
            ui.mouse.middlePressed = true;
        else if (action == GLFW_RELEASE)
            ui.mouse.middlePressed = false;
    }

} // mouseButton_callback

void mouseScroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    UI &ui = *reinterpret_cast<UI *>(glfwGetWindowUserPointer(window));
    ui.mouse.offset = {float(xoffset), float(yoffset)};

} // mouseScroll_callback

/*****************************************************************************/
/*****************************************************************************/

UI::UI(const String &name, uint32_t width, uint32_t height)
    : screenTitle(name), screenDim(width, height)
{
    // Setup window
    glfwSetErrorCallback([](int error, const char *description) -> void {
        std::cout << "ERROR (glfw): " << error << ", " << description << std::endl;
        exit(0);
    });

    if (!glfwInit())
    {
        std::cerr << "ERROR (glfw): Couldn't start glfw!!!\n";
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Mac OS only
#endif

    // Generating resizable window
    main_window = glfwCreateWindow(screenDim.x, screenDim.y, screenTitle.c_str(), NULL, NULL);
    if (main_window == NULL)
    {
        std::cerr << "ERROR: Failed to create GLFW window!!\n";
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(main_window);
    glfwSwapInterval(1); // synchronize with screen updates

    // Initialize OPENGL loader
    if (gladLoadGL() == 0)
    {
        std::cerr << "ERROR (glad): Failed to initialize OpenGL loader!!!\n";
        exit(-1);
    }

    // Setup viewport -> opengl is going transform final coordinates into this range
    gl_call(glad_glViewport(0, 0, screenDim.x, screenDim.y));

    // ///////////////////////////////////////////////////////////////////////////
    // // HANDLING EVENTS
    glfwSetWindowUserPointer(main_window, this);
    glfwSetCursorPosCallback(main_window, mousePos_callback);
    glfwSetMouseButtonCallback(main_window, mouseButton_callback);
    glfwSetScrollCallback(main_window, mouseScroll_callback);
    glfwSetWindowSizeCallback(main_window, winResize_callback);

    ///////////////////////////////////////////////////////////////////////////
    // SETUP DEAR IMGUI/IMPLOT CONTEXT

    // Setup Platform/Renderer bindings
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui::StyleColorsClassic();

    // Floating windows off main windows
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    io.IniFilename = "gptool.ini";

    ImGui_ImplGlfw_InitForOpenGL(main_window, true);
    ImGui_ImplOpenGL3_Init("#version 410 core"); // Mac supports only up to 410

    ////////////////////////////////////////////////////////////////////////////
    // INITIALIZING CONTEXT DEPENDENT VARIABLES

    shader.loadShaders();
    quad.generateObj();

    ///////////////////////////////////////////////////////////////////////////
    // Setup fonts

    fonts.loadFont("regular", "Open_Sans/OpenSans-Regular.ttf", 18.0);
    fonts.loadFont("bold", "Open_Sans/OpenSans-Bold.ttf", 18.0);
    fonts.setDefault("regular");

    mWindows["inbox"] = std::make_unique<winInboxMessage>(&mail);
    mWindows["dialog"] = std::make_unique<FileDialog>();

} // constructor

UI::~UI(void)
{
    tex.cleanUp();
    quad.cleanUp();
    shader.cleanUp();
    FBuffer.clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();

    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwTerminate();

} // destructor

void UI::mainLoop(void (*onUserUpdate)(UI &), void (*controls)(UI &), void (*ImGuiMenuLayer)(UI &),
                  void (*ImGuiLayer)(UI &))
{
    glfwSetTime(0);
    double t0 = glfwGetTime();

    // resetProgram(data);
    while (!glfwWindowShouldClose(main_window))
    {
        // reset events
        resizeEvent = false;
        mouse.leftButton = false;
        mouse.middleButton = false;
        mouse.rightButton = false;
        mouse.offset = {0.0, 0.0};

        glfwPollEvents();

        controls(*this);

        // Create a secondary buffer for swap
        onUserUpdate(*this);

        // Setup a new frame for imgui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
        window_flags |= ImGuiWindowFlags_MenuBar;
        window_flags |= ImGuiWindowFlags_NoDocking;
        window_flags |= ImGuiWindowFlags_NoTitleBar;
        window_flags |= ImGuiWindowFlags_NoCollapse;
        window_flags |= ImGuiWindowFlags_NoResize;
        window_flags |= ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        window_flags |= ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        static bool p_open = true;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", &p_open, window_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));

        if (ImGui::BeginMenuBar())
        {
            ImGuiMenuLayer(*this);
            ImGui::EndMenuBar();
        }

        // Here goes the implementation for user interface
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.0, 0.0, 0.0, 1.0}); // solid background
        ImGuiLayer(*this);
        ImGui::PopStyleColor();

        // Ending dockspace
        ImGui::End();

        // Render ImGui // It calls endFrame automatically
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Allows redering of external floating windows
        GLFWwindow *backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);

        // Swap secondary buffer to screen
        glfwSwapBuffers(main_window);

        // Calculating elapsed time for smooth controls
        double tf = glfwGetTime();
        elapsedTime = float(tf - t0);
        t0 = tf;

    } // main-loop

} // mainLoop
#include "gptool.h"

#include <thread>

GPTool::GPTool(void)
{
    initialize("GShader", 1200 * DPI_FACTOR, 800 * DPI_FACTOR);
    shader = std::make_unique<Shader>();
    quad = std::make_unique<Quad>();

    manager = std::make_unique<PluginManager>(&fonts);
    fBuffer["viewport"] = std::make_unique<Framebuffer>(1200, 800);
} // function

void GPTool::onUserUpdate(float deltaTime)
{

    bool check = keyboard[Key::LEFT_CONTROL] == Event::PRESS;
    check |= keyboard[Key::RIGHT_CONTROL] == Event::PRESS;
    check &= keyboard['O'] == Event::RELEASE;
    if (check)
        dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {".tif", ".ome.tif"}, this, [](void *ptr) -> void {
            GPTool *tool = (GPTool *)ptr;
            tool->openMovie(tool->dialog.getPath());
        });

    if (viewport_hover)
        viewport_function(deltaTime);

    ///////////////////////////////////////////////////////
    // Renderering

    fBuffer["viewport"]->bind();
    glad_glClear(GL_COLOR_BUFFER_BIT);
    glad_glClearColor(0.6, 0.6, 0.6, 1.0);

    glm::mat4 trf = camera.getViewMatrix();
    const glm::vec2 &size = fBuffer["viewport"]->getSize();

    shader->useProgram("basic");
    shader->setMatrix4f("u_transform", glm::value_ptr(trf));

    quad->draw();

    fBuffer["viewport"]->unbind();

} // function

void GPTool::ImGuiLayer(void)
{
    manager->showHeader();
    manager->showProperties();

    ///////////////////////////////////////////////////////

    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoTitleBar);

    // Check if it needs to resize
    ImVec2 port = ImGui::GetContentRegionAvail();
    ImGui::Image((void *)(uintptr_t)fBuffer["viewport"]->getID(), port);

    glm::vec2 view = fBuffer["viewport"]->getSize();
    if (port.x != view.x || port.y != view.y)
    {
        fBuffer["viewport"] = std::make_unique<Framebuffer>(port.x, port.y);
        camera.setAspectRatio(port.y / port.x);
    }

    // Checking if anchoring position changed
    ImVec2 pos = ImGui::GetItemRectMin();
    fBuffer["viewport"]->setPosition(pos.x - window.position.x, pos.y - window.position.y);

    // Check if mouse is on viewport
    viewport_hover = ImGui::IsWindowHovered();

    ImGui::End();

    ///////////////////////////////////////////////////////

    dialog.showDialog();
    mbox.showMessages();

    // // External windows
    // for (auto &win : ui.mWindows)
    //     if (win.second->active)
    //         win.second->display();

} // function

void GPTool::ImGuiMenuLayer(void)
{
    // MainStruct &data = *reinterpret_cast<MainStruct *>(ui.getUserData());
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open movie..."))
            dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {".tif", ".ome.tif"}, this, [](void *ptr) -> void {
                GPTool *tool = (GPTool *)ptr;
                tool->openMovie(tool->dialog.getPath());
            });

        bool enable = false;
        if (ImGui::MenuItem("Load trajectories...", NULL, nullptr, enable))
        {
        }

        if (ImGui::MenuItem("Save as ...", NULL, nullptr, enable))
        {
        }

        if (ImGui::MenuItem("Exit"))
            closeApp();

        ImGui::EndMenu();
    } // file-menu

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    if (ImGui::BeginMenu("About"))
    {
        if (ImGui::MenuItem("Show mailbox"))
            mbox.setActive();

        // if (ImGui::MenuItem("How to cite"))
        // {
        //     ui.mail.createMessage<MSG_Info>("When the paper is out");
        //     ui.mWindows["inbox"]->show();
        // }

        // if (ImGui::MenuItem("Documentation"))
        // {
        //     ui.mail.createMessage<MSG_Info>("Link to github later");
        //     ui.mWindows["inbox"]->show();
        // }

        ImGui::EndMenu();
    }
} // function

/////////////////////////////////////

void GPTool::viewport_function(float deltaTime)
{

    // move camera -- add roi points
    if (mouse[Mouse::LEFT] == Event::PRESS)
    {
        glm::vec2 dr = mouse.offset * deltaTime;
        camera.moveHorizontal(dr.x);
        camera.moveVertical(dr.y);
    } // left-button-pressed

    // zoom
    if (mouse.wheel.y > 0.0f)
        camera.moveFront(deltaTime);

    if (mouse.wheel.y < 0.0f)
        camera.moveBack(deltaTime);

} // controls

void GPTool::openMovie(const String &path)
{

    std::thread([&](void) -> void {
        MoviePlugin *movie = new MoviePlugin(path, this);
        if (movie->successful())
        {
            manager = std::make_unique<PluginManager>(&fonts);
            manager->addPlugin("MOVIE", movie);
            manager->setActive("MOVIE");

            // TODO: add all the other plugins
        }
        else
            delete movie;
    }).detach();
}

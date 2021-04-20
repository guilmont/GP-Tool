#include "gptool.h"

#include <thread>

GPTool::GPTool(void)
{
    initialize("GShader", 1200 * DPI_FACTOR, 800 * DPI_FACTOR);
    shader = std::make_unique<Shader>();
    quad = std::make_unique<Quad>();

    uint32_t cor = 0x999999ff;

    manager = std::make_unique<PluginManager>(&fonts);
    viewBuf = std::make_unique<Framebuffer>(1, 1);

} // function

void GPTool::onUserUpdate(float deltaTime)
{
    // Control signal hanfling

    bool check = keyboard[Key::LEFT_CONTROL] == Event::PRESS;
    check |= keyboard[Key::RIGHT_CONTROL] == Event::PRESS;
    check &= keyboard['O'] == Event::RELEASE;
    if (check)
        dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {".tif", ".ome.tif"}, this, [](void *ptr) -> void {
            GPTool *tool = (GPTool *)ptr;
            tool->openMovie(tool->dialog.getPath());
        });

    if (viewport_hover)
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
    }

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////
    // Handling plugins

    ///////////////////////////////////////////////////////
    // Renderering
    // if (!updateViewport)
    //     return;

    viewBuf->bind();
    glad_glClear(GL_COLOR_BUFFER_BIT);
    glad_glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

    shader->useProgram("viewport");
    manager->updateAll(deltaTime);
    quad->draw();
    viewBuf->unbind();

    // std::cout << deltaTime << std::endl;

} // function

void GPTool::ImGuiLayer(void)
{
    manager->showHeader();
    manager->showProperties();
    manager->showWindows();

    ///////////////////////////////////////////////////////

    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoTitleBar);

    // Check if it needs to resize
    ImVec2 port = ImGui::GetContentRegionAvail();
    ImGui::Image((void *)(uintptr_t)viewBuf->getID(), port);

    glm::vec2 view = viewBuf->getSize();
    if (port.x != view.x || port.y != view.y)
    {
        viewBuf = std::make_unique<Framebuffer>(uint32_t(port.x), uint32_t(port.y));
        camera.setAspectRatio(port.x / port.y);
    }

    // Checking if anchoring position changed
    ImVec2 pos = ImGui::GetItemRectMin();
    viewBuf->setPosition(pos.x - window.position.x, pos.y - window.position.y);

    // Check if mouse is on viewport
    viewport_hover = ImGui::IsWindowHovered();

    ImGui::End();

    ///////////////////////////////////////////////////////

    dialog.showDialog();
    mbox.showMessages();

} // function

void GPTool::ImGuiMenuLayer(void)
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open movie..."))
            dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {".tif", ".ome.tif"}, this, [](void *ptr) -> void {
                GPTool *tool = (GPTool *)ptr;
                tool->openMovie(tool->dialog.getPath());
            });

        TrajPlugin *traj = (TrajPlugin *)manager->getPlugin("TRAJECTORY");
        if (ImGui::MenuItem("Load trajectories...", NULL, nullptr, traj != nullptr))
            traj->loadTracks();

        // if (ImGui::MenuItem("Save as ...", NULL, nullptr, enable))
        // {
        // }

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

        char buf[64] = {0};
        sprintf_s(buf, "FPS: %.0f", ImGui::GetIO().Framerate);
        ImGui::MenuItem(buf);

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

void GPTool::openMovie(const String &path)
{

    std::thread([&](void) -> void {
        MoviePlugin *movpl = new MoviePlugin(path, this);
        if (movpl->successful())
        {
            camera.reset();

            // Setup plugin manager
            // pluygin pointers will be owned and destroyed by manager
            manager = std::make_unique<PluginManager>(&fonts);

            // Including movie plugin into the manager
            manager->addPlugin("MOVIE", movpl);
            manager->setActive("MOVIE");

            // Determine if we need the alignment plugin
            const Movie *movie = movpl->getMovie();
            if (movie->getMetadata().SizeC > 1)
            {
                AlignPlugin *alg = new AlignPlugin(movie, this);
                manager->addPlugin("ALIGNMENT", alg);
            }

            // Let's also activate the trajectory plugin
            TrajPlugin *traj = new TrajPlugin(movie, this);
            manager->addPlugin("TRAJECTORY", traj);
        }
        else
            delete movpl;
    }).detach();
}

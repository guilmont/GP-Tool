#include "gptool.h"

#include <thread>

GPTool::GPTool(void)
{
    initialize("GP-Tool", 1200 * DPI_FACTOR, 800 * DPI_FACTOR);
    shader = std::make_unique<Shader>();
    quad = std::make_unique<Quad>();

    uint32_t cor = 0x999999ff;

    manager = std::make_unique<PluginManager>(this);
    viewBuf = std::make_unique<Framebuffer>(1, 1);

} // function

void GPTool::onUserUpdate(float deltaTime)
{
    // key combination for opening movie
    bool check = keyboard[Key::LEFT_CONTROL] == Event::PRESS || keyboard[Key::RIGHT_CONTROL] == Event::PRESS;
    check &= keyboard['O'] == Event::RELEASE;
    if (check)
        dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {".tif", ".ome.tif"}, this, [](const std::string &path, void *ptr) -> void
                            {
                                GPTool *tool = (GPTool *)ptr;
                                tool->openMovie(path);
                            });

    ///////////////////////////////////////////////////////
    // key combination to save data
    check = keyboard[Key::LEFT_CONTROL] == Event::PRESS || keyboard[Key::RIGHT_CONTROL] == Event::PRESS;
    check &= keyboard['S'] == Event::RELEASE;
    if (check)
        dialog.createDialog(GDialog::SAVE, "Save data...", {".json"}, manager.get(),
                            [](const std::string &path, void *ptr) -> void
                            {
                                std::thread([](PluginManager *manager, const std::string &path) -> void
                                            { manager->saveJSON(path); },
                                            (PluginManager *)ptr, path)
                                    .detach();
                            });

    ///////////////////////////////////////////////////////
    // key combination open trajectory
    check = keyboard[Key::LEFT_CONTROL] == Event::PRESS || keyboard[Key::RIGHT_CONTROL] == Event::PRESS;
    check &= keyboard['T'] == Event::RELEASE;
    if (check)
    {
        TrajPlugin *traj = (TrajPlugin *)manager->getPlugin("TRAJECTORY");
        traj->loadTracks();
    }

    if (viewport_hover)
    {
        // move camera
        if (mouse[Mouse::LEFT] == Event::PRESS)
        {
            glm::vec2 dr = mouse.offset * deltaTime;
            camera.moveHorizontal(dr.x);
            camera.moveVertical(dr.y);
        }

        // zoom
        if (mouse.wheel.y > 0.0f)
            camera.moveFront(deltaTime);

        if (mouse.wheel.y < 0.0f)
            camera.moveBack(deltaTime);
    }

    ///////////////////////////////////////////////////////
    // Renderering

    viewBuf->bind();

    // Clearing buffer
    glad_glClear(GL_COLOR_BUFFER_BIT);
    glad_glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

    shader->useProgram("viewport"); // chooseing rendering program
    manager->updateAll(deltaTime);  // Let all plugins do what they need
    quad->draw();                   // rendering final image

    viewBuf->unbind();

} // function

void GPTool::ImGuiLayer(void)
{
    // ImGui::ShowDemoWindow();
    // ImPlot::ShowDemoWindow();

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
            dialog.createDialog(GDialog::OPEN, "Choose TIF file...",
                                {".tif", ".ome.tif"}, this,
                                [](const std::string &path, void *ptr) -> void
                                {
                                    GPTool *tool = (GPTool *)ptr;
                                    tool->openMovie(path);
                                });

        TrajPlugin *traj = (TrajPlugin *)manager->getPlugin("TRAJECTORY");
        if (ImGui::MenuItem("Load trajectories...", NULL, nullptr, traj != nullptr))
            traj->loadTracks();

        if (ImGui::MenuItem("Save as ...", NULL, nullptr))
            dialog.createDialog(GDialog::SAVE, "Choose TIF file...", {".json"}, manager.get(),
                                [](const std::string &path, void *ptr) -> void
                                {
                                    std::thread([](PluginManager *manager, const std::string &path) -> void
                                                { manager->saveJSON(path); },
                                                (PluginManager *)ptr, path)
                                        .detach();
                                });

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
        sprintf(buf, "FPS: %.0f", ImGui::GetIO().Framerate);
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

    std::thread([](GPTool *tool, const String &path) -> void
                {
                    MoviePlugin *movpl = new MoviePlugin(path, tool);
                    if (movpl->successful())
                    {
                        tool->camera.reset();

                        // Setup plugin manager
                        // pluygin pointers will be owned and destroyed by manager
                        tool->manager = std::make_unique<PluginManager>(tool);

                        // Including movie plugin into the manager
                        tool->manager->addPlugin("MOVIE", movpl);
                        tool->manager->setActive("MOVIE");

                        // Determine if we need the alignment plugin
                        const Movie *movie = movpl->getMovie();
                        if (movie->getMetadata().SizeC > 1)
                        {
                            AlignPlugin *alg = new AlignPlugin(movie, tool);
                            tool->manager->addPlugin("ALIGNMENT", alg);
                        }

                        // Let's also activate:

                        // trajectory plugin
                        TrajPlugin *traj = new TrajPlugin(movie, tool);
                        tool->manager->addPlugin("TRAJECTORY", traj);

                        // Gaussian process plugin
                        GPPlugin *gp = new GPPlugin(tool);
                        tool->manager->addPlugin("GPROCESS", gp);
                    }
                    else
                        delete movpl;
                },
                this, path)
        .detach();
}
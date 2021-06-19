#include "gptool.h"


GPTool::GPTool(void)
{
    initialize("GP-Tool", 1200 * DPI_FACTOR, 800 * DPI_FACTOR);
   
    uint32_t cor = 0x999999ff;
    quad = std::make_unique<GRender::Quad>();
    viewBuf = std::make_unique<GRender::Framebuffer>(1, 1);

    // Initializing shaders
    fs::path shaderPath(SHADER_PATH);
    shader.loadShader("histogram", (shaderPath / "basic.vtx.glsl").string(), (shaderPath / "histogram.frag.glsl").string());
    shader.loadShader("viewport", (shaderPath / "basic.vtx.glsl").string(), (shaderPath / "viewport.frag.glsl").string());

    // initializing plugins
    plugins["ALIGNMENT"] = nullptr;
    plugins["GPROCESS"] = nullptr;
    plugins["MOVIE"] = nullptr;
    plugins["TRAJECTORY"] = nullptr;

} // function

GPTool::~GPTool(void)
{
    // Releasing all plugins
    for (auto& [name, ptr] : plugins)
        if (ptr)
            ptr.release();
}

void GPTool::showHeader(void)
{
    ImGui::Begin("Plugins");

    float width = ImGui::GetContentRegionAvailWidth();
    ImVec2 buttonSize{ width, 40 * DPI_FACTOR };

    ImVec4
        deactivated{ 0.13f, 0.16f, 0.3f, 1.f },
        chosenColor{ 0.1f, 0.6f, 0.1f, 1.0f },
        hoverColor{ 0.2f, 0.7f, 0.2f, 1.0f };

    fonts.push("bold");

    for (auto const& [name, ptr] : plugins)
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

    fonts.pop();

    ImGui::End();
}

void GPTool::showWindows(void)
{
    for (auto& [name, ptr] : plugins)
        if (ptr)
            ptr->showWindows();
}

void GPTool::showProperties(void)
{
    if (pActive)
        pActive->showProperties();
    else
    {
        ImGui::Begin("Properties");
        ImGui::Text("No movie loaded");
        ImGui::End();
    }
}

void GPTool::updateAll(float deltaTime)
{
    for (auto& [name, ptr] : plugins)
        if (ptr)
            ptr->update(deltaTime);
}

void GPTool::addPlugin(const std::string& name, Plugin* plugin) { plugins[name].reset(plugin); }

Plugin* GPTool::getPlugin(const std::string& name) { return plugins[name].get(); }

void GPTool::setActive(const std::string& name) { pActive = plugins[name].get(); }



void GPTool::onUserUpdate(float deltaTime)
{
    bool ctrl = keyboard[GKey::LEFT_CONTROL] == GEvent::PRESS || keyboard[GKey::RIGHT_CONTROL] == GEvent::PRESS,
         O = keyboard['O'] == GEvent::RELEASE,
         S = keyboard['S'] == GEvent::RELEASE,
         T = keyboard['T'] == GEvent::RELEASE;

    // key combination for opening movie
    if (ctrl & O)
        dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {"tif", "ome.tif"}, this, 
            [](const std::string &path, void *ptr) -> void {reinterpret_cast<GPTool *>(ptr)->openMovie(path); });

    //// key combination to save data
    //if (ctrl & S)
    //    dialog.createDialog(GDialog::SAVE, "Save data...", {".json"}, this,
    //                        [](const std::string &path, void *ptr) -> void
    //                        {
    //                            std::thread([](PluginManager *manager, const std::string &path) -> void
    //                                        { manager->saveJSON(path); },
    //                                        (PluginManager *)ptr, path)
    //                                .detach();
    //                        });

    ///////////////////////////////////////////////////////
    // key combination open trajectory
    if ((ctrl & T) && plugins["TRAJECTORY"])
        reinterpret_cast<TrajPlugin*>(plugins["TRAJECTORY"].get())->loadTracks();
            

    if (viewport_hover)
    {
        // move camera
        if (mouse[GMouse::LEFT] == GEvent::PRESS)
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

    shader.useProgram("viewport");  // chooseing rendering program
    updateAll(deltaTime);           // Updateing all plugins
    quad->draw();                   // rendering final image
    viewBuf->unbind();

} // function

void GPTool::ImGuiLayer(void)
{
    // ImGui::ShowDemoWindow();
    // ImPlot::ShowDemoWindow();

    // Plugin functions
    showHeader();
    showProperties();
    showWindows();

    dialog.showDialog();
    mbox.showMessages();

    /////////////////////////////////////////////////////////

    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoTitleBar);

    // Check if it needs to resize
    ImVec2 port = ImGui::GetContentRegionAvail();
    ImGui::Image((void *)(uintptr_t)viewBuf->getID(), port);

    glm::vec2 view = viewBuf->getSize();
    if (port.x != view.x || port.y != view.y)
    {
        viewBuf = std::make_unique<GRender::Framebuffer>(uint32_t(port.x), uint32_t(port.y));
        camera.setAspectRatio(port.x / port.y);
    }

    // Checking if anchoring position changed
    ImVec2 pos = ImGui::GetItemRectMin();
    viewBuf->setPosition(pos.x - window.position.x, pos.y - window.position.y);

    // Check if mouse is on viewport
    viewport_hover = ImGui::IsWindowHovered();

    ImGui::End();


}

void GPTool::ImGuiMenuLayer(void)
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Open movie..."))
            dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {"tif", "ome.tif"}, this,
                                [](const std::string &path, void *ptr) -> void { reinterpret_cast<GPTool *>(ptr)->openMovie(path); });

        if (ImGui::MenuItem("Load trajectories...", NULL, nullptr, plugins["TRAJECTORY"] != nullptr))
            reinterpret_cast<TrajPlugin*>(plugins["TRAJECTORY"].get())->loadTracks();

    //    if (ImGui::MenuItem("Save as ...", NULL, nullptr))
    //        dialog.createDialog(GDialog::SAVE, "Choose TIF file...", {".json"}, manager.get(),
    //                            [](const std::string &path, void *ptr) -> void
    //                            {
    //                                std::thread([](PluginManager *manager, const std::string &path) -> void
    //                                            { manager->saveJSON(path); },
    //                                            (PluginManager *)ptr, path)
    //                                    .detach();
    //                            });

        if (ImGui::MenuItem("Exit"))
            closeApp();

        ImGui::EndMenu();
    } 

    if (ImGui::BeginMenu("About"))
    {
        if (ImGui::MenuItem("Show mailbox"))
            mbox.setActive();

          if (ImGui::MenuItem("How to cite"))
            mbox.createInfo("Refer to paper at https://doi.org/10.1101/2021.03.16.435699");

        if (ImGui::MenuItem("Documentation"))
            mbox.createInfo("View complete documentation at https://github.com/guilmont/GP-Tool");

        ImGui::EndMenu();
    }

} 


/////////////////////////////////////

void GPTool::openMovie(const std::string &path)
{

    std::thread([](GPTool *tool, const std::string &path) -> void {
       
        MoviePlugin *movpl = new MoviePlugin(path, tool);

        if (movpl->successful())
        {
            tool->camera.reset();


            // Including movie plugin into the manager
            tool->addPlugin("MOVIE", movpl);
            tool->setActive("MOVIE");

            // Determine if we need the alignment plugin
            const Movie *movie = movpl->getMovie();
            if (movie->getMetadata().SizeC > 1)
            {
                AlignPlugin *alg = new AlignPlugin(movie, tool);
                tool->addPlugin("ALIGNMENT", alg);
            }

            // trajectory plugin
            TrajPlugin *traj = new TrajPlugin(movie, tool);
            tool->addPlugin("TRAJECTORY", traj);

            // Gaussian process plugin
            GPPlugin *gp = new GPPlugin(tool);
            tool->addPlugin("GPROCESS", gp);
        }
        else
            delete movpl;

    },this, path).detach();
}

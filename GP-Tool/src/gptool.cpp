#include "gptool.h"


GPTool::GPTool(void)
{
    fs::path assets(ASSETS);

    initialize("GP-Tool", 1200, 800, assets / "layout.ini");
    viewBuf = std::make_unique<GRender::Framebuffer>(1,1);

    // Initializing shaders
    fs::path shaderPath(assets / "shaders");
    shader.loadShader("histogram", (shaderPath / "basic.vtx.glsl").string(), (shaderPath / "histogram.frag.glsl").string());
    shader.loadShader("viewport", (shaderPath / "basic.vtx.glsl").string(), (shaderPath / "viewport.frag.glsl").string());
    shader.loadShader("circles", (shaderPath / "spot.vtx.glsl").string(), (shaderPath / "spot.frag.glsl").string());
    shader.loadShader("roi", (shaderPath / "basic.vtx.glsl").string(), (shaderPath / "roi.frag.glsl").string());

    // initializing plugins
    plugins["ALIGNMENT"] = nullptr;
    plugins["GPROCESS"] = nullptr;
    plugins["MOVIE"] = nullptr;
    plugins["TRAJECTORY"] = nullptr;

}

GPTool::~GPTool(void)
{
    // Releasing all plugins
    for (auto &[name, ptr] : plugins)
        if (ptr)
            ptr.release();
}

void GPTool::showHeader(void)
{
    const ImVec2 wp = ImGui::GetMainViewport()->WorkPos;
    const ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowPos({wp.x + ws.x * plPos.x, wp.y + ws.y * plPos.y}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ws.x * plSize.x, ws.y * plSize.y}, ImGuiCond_FirstUseEver);

    ImGui::Begin("Plugins");

    float width = ImGui::GetContentRegionAvailWidth();
    ImVec2 buttonSize{width, 40 * GRender::DPI_FACTOR};

    ImVec4
        deactivated{0.13f, 0.16f, 0.3f, 1.f},
        chosenColor{0.1f, 0.6f, 0.1f, 1.0f},
        hoverColor{0.2f, 0.7f, 0.2f, 1.0f};

    fonts.push("bold");

    for (auto const &[name, ptr] : plugins)
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
    for (auto &[name, ptr] : plugins)
        if (ptr)
            ptr->showWindows();
}

void GPTool::showProperties(void)
{

    const ImVec2 wp = ImGui::GetMainViewport()->WorkPos;
    const ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowPos({wp.x + ws.x * propPos.x, wp.y + ws.y * propPos.y}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ws.x * propSize.x, ws.y * propSize.y}, ImGuiCond_FirstUseEver);

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
    if (plugins["MOVIE"])
        plugins["MOVIE"]->update(deltaTime);  // The alignment plugin will be called from within
    
    if (plugins["TRAJECTORY"])
        plugins["TRAJECTORY"]->update(deltaTime);

    if (plugins["GPROCESS"])
        plugins["GPROCESS"]->update(deltaTime);
}

void GPTool::addPlugin(const std::string &name, Plugin *plugin) { plugins[name].reset(plugin); }

Plugin *GPTool::getPlugin(const std::string &name) { return plugins[name].get(); }

void GPTool::setActive(const std::string &name) { pActive = plugins[name].get(); }

void GPTool::onUserUpdate(float deltaTime)
{
    bool 
        ctrl = keyboard[GKey::LEFT_CONTROL] == GEvent::PRESS || keyboard[GKey::RIGHT_CONTROL] == GEvent::PRESS,
        ctrlRep = keyboard[GKey::LEFT_CONTROL] == GEvent::REPEAT || keyboard[GKey::RIGHT_CONTROL] == GEvent::REPEAT,
        O = keyboard['O'] == GEvent::RELEASE,
        S = keyboard['S'] == GEvent::RELEASE,
        T = keyboard['T'] == GEvent::RELEASE;

    // key combination for opening movie
    if (ctrl & O)
        dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {"tif", "ome.tif"}, this,
                            [](const fs::path &path, void *ptr) -> void { reinterpret_cast<GPTool *>(ptr)->openMovie(path); });

    // key combination to save data
    if (ctrl & S)
        dialog.createDialog(GDialog::SAVE, "Save data...", {"json"}, this,
                            [](const fs::path &path, void *ptr) -> void
                            {
                                std::thread([](GPTool *tool, const fs::path &path) -> void{ tool->saveJSON(path); }, reinterpret_cast<GPTool *>(ptr), path).detach();
                            });

    ///////////////////////////////////////////////////////
    // key combination open trajectory
    if ((ctrl & T) && plugins["TRAJECTORY"])
        reinterpret_cast<TrajPlugin *>(plugins["TRAJECTORY"].get())->loadTracks();

    if (viewport_hover)
    {
        // move camera
        if (mouse[GMouse::LEFT] == GEvent::PRESS)
        {
            glm::vec2 dr = mouse.offset * deltaTime;
            camera.moveHorizontal(dr.x);
            camera.moveVertical(dr.y);
        }

        // Moving frames
        if (plugins["MOVIE"])
        {
            MoviePlugin* mov = reinterpret_cast<MoviePlugin*>(plugins["MOVIE"].get());
            uint64_t nFrames = mov->getMovie()->getMetadata().SizeT;

            if (keyboard[GKey::RIGHT] == GEvent::PRESS)
            {
                mov->current_frame += (mov->current_frame + 1 == nFrames) ? 0 : 1;
                mov->updateDisplay();
            }

            else if (keyboard[GKey::LEFT] == GEvent::PRESS)
            {
                mov->current_frame -= (mov->current_frame == 0) ? 0 : 1;
                mov->updateDisplay();
            }

            else if (keyboard[GKey::UP] == GEvent::PRESS)
            {
                mov->current_frame = 0;
                mov->updateDisplay();
            }

            else if (keyboard[GKey::DOWN] == GEvent::PRESS)
            {
                mov->current_frame = nFrames - 1;
                mov->updateDisplay();
            }
        }


        // Trajectory roi stuff
        
        if (plugins["TRAJECTORY"] && (ctrlRep | ctrl))
        {
                bool active = plugins["TRAJECTORY"]->isActive(); // if trajectories are loaded

                // Adding vertex to roi
                if ((mouse[GMouse::LEFT] == GEvent::RELEASE) & active)
                {
                    glm::vec2 click = getClickPosition();  // Reference to viewport
                    click = { click.x + 0.5f, click.y + 0.5f }; // Converting to image coordinates
                    reinterpret_cast<TrajPlugin*>(plugins["TRAJECTORY"].get())->roi.addPosition(click);
                }

                // Removing vertex from roi
                else if ((mouse[GMouse::RIGHT] == GEvent::RELEASE) & active)
                {
                    glm::vec2 click = getClickPosition();
                    click = { click.x + 0.5f, click.y + 0.5f };

                    reinterpret_cast<TrajPlugin*>(plugins["TRAJECTORY"].get())->roi.removePosition(click);
                }

                // Moving roi vertex
                else if (mouse[GMouse::MIDDLE] == GEvent::PRESS || mouse[GMouse::MIDDLE] == GEvent::REPEAT)
                {
                    glm::vec2 click = getClickPosition();
                    click = { click.x + 0.5f, click.y + 0.5f };
                    reinterpret_cast<TrajPlugin*>(plugins["TRAJECTORY"].get())->roi.movePosition(click);
                }
        }

        // zoom
        if (mouse.wheel.y > 0.0f)
            camera.moveFront(deltaTime);

        if (mouse.wheel.y < 0.0f)
            camera.moveBack(deltaTime);
    }

    ///////////////////////////////////////////////////////
    // Renderering

    // Clearing buffer
    viewBuf->bind();
    glad_glClear(GL_COLOR_BUFFER_BIT);
    glad_glClearColor(0.6f, 0.6f, 0.6f, 1.0f);
    viewBuf->unbind();

    // Updating all plugins
    updateAll(deltaTime);
    
} // function

void GPTool::ImGuiLayer(void)
{
    // Plugin functions
    showHeader();
    showProperties();
    showWindows();

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////

    const ImVec2 wp = ImGui::GetMainViewport()->WorkPos;
    const ImVec2 ws = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowPos({wp.x + ws.x * vwPos.x, wp.y + ws.y * vwPos.y}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize({ws.x * vwSize.x, ws.y * vwSize.y}, ImGuiCond_FirstUseEver);

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
        if (ImGui::MenuItem("Open movie...", "Ctrl+O"))
            dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {"tif", "ome.tif"}, this,
                                [](const fs::path &path, void *ptr) -> void { reinterpret_cast<GPTool *>(ptr)->openMovie(path); });

        if (ImGui::MenuItem("Load trajectories...", "Ctrl+T", nullptr, plugins["TRAJECTORY"] != nullptr))
            reinterpret_cast<TrajPlugin *>(plugins["TRAJECTORY"].get())->loadTracks();

        if (ImGui::MenuItem("Save as ...", "Ctrl+S"))
            dialog.createDialog(GDialog::SAVE, "Choose TIF file...", {"json"}, this,
                                [](const fs::path &path, void *ptr) -> void
                                {
                                    std::thread([](GPTool *tool, const fs::path &path) -> void { tool->saveJSON(path); }, reinterpret_cast<GPTool *>(ptr), path).detach();
                                });

        if (ImGui::MenuItem("Exit"))
            closeApp();

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("About"))
    {

        if (ImGui::MenuItem("Show mailbox"))
            mailbox.setActive();

        if (GRender::DPI_FACTOR == 1)
        {
            if (ImGui::MenuItem("Set HIDPI"))
                scaleSizes(2.0f);
        }
        else
        {
            if (ImGui::MenuItem("Unset HIDPI"))
                scaleSizes(1.0f);
        }

        if (ImGui::MenuItem("How to cite"))
            mailbox.createInfo("Refer to paper at https://doi.org/10.1038/s41467-021-26466-7");

        if (ImGui::MenuItem("Documentation"))
            mailbox.createInfo("View complete documentation at https://github.com/guilmont/GP-Tool");

        ImGui::EndMenu();
    }
}

/////////////////////////////////////

void GPTool::openMovie(const fs::path &path)
{
    std::thread(
        [](GPTool *tool, const fs::path &path) -> void
        {
            MoviePlugin *movpl = new MoviePlugin(path, tool);

            if (movpl->successful())
            {
                tool->camera.getPosition() = { 0.0f, 0.0f, 0.8f };
                
                // Including movie plugin into the manager
                tool->addPlugin("MOVIE", movpl);
                tool->setActive("MOVIE");

                // Determine if we need the alignment plugin
                GPT::Movie *movie = movpl->getMovie();
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
        },
        this, path)
        .detach();
}

void GPTool::saveJSON(const fs::path &path)
{

    Json::Value output;

    Plugin *pgl = plugins["MOVIE"].get();
    if (pgl)
        pgl->saveJSON(output);
    else
    {
        mailbox.createInfo("Nothing to be save!!");
        return;
    }

    pgl = plugins["ALIGNMENT"].get();
    if (pgl)
        pgl->saveJSON(output["Alignment"]);

    pgl = plugins["TRAJECTORY"].get();
    if (pgl)
    {
        Json::Value aux;
        if (pgl->saveJSON(aux))
            output["Trajectory"] = std::move(aux);
    }

    pgl = plugins["GPROCESS"].get();
    if (pgl)
    {
        Json::Value aux;
        if (pgl->saveJSON(aux))
            output["GProcess"] = std::move(aux);
    }

    std::ofstream arq(path);
    arq << output;
    arq.close();

    mailbox.createInfo("File saved to '" + path.string() + "'");

} // saveJSON

glm::vec2 GPTool::getClickPosition(void)
{
    const glm::vec2& pos = viewBuf->getPosition();
    const glm::vec2& size = viewBuf->getSize();
    glm::vec2 click = { 2.0f * (mouse.position.x - pos.x) / size.x - 1.0f,
                       2.0f * (mouse.position.y - pos.y) / size.y - 1.0f };


    const GPT::Metadata& meta = reinterpret_cast<MoviePlugin*>(plugins["MOVIE"].get())->getMovie()->getMetadata();
    float iratio = float(meta.SizeX) / float(meta.SizeY);
    const glm::vec3& cpos = camera.getPosition();

    return glm::vec2{ click.x * cpos.z + cpos.x, (click.y * cpos.z + cpos.y) * iratio };
}

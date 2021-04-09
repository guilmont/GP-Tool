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

    MoviePlugin *plmov = (MoviePlugin *)manager->getPlugin("MOVIE");

    if (!texture && plmov)
    {
        // In case program is fresh or it was reset
        const Metadata &meta = plmov->getMovie()->getMetadata();

        texture = std::make_unique<Texture>();
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
            texture->create(meta.SizeX, meta.SizeY);
    }

    manager->updateAll(deltaTime);

    ///////////////////////////////////////////////////////
    // Renderering
    // if (!updateViewport)
    //     return;

    viewBuf->bind();
    glad_glClear(GL_COLOR_BUFFER_BIT);
    glad_glClearColor(0.6, 0.6, 0.6, 1.0);

    if (plmov)
    {
        const Metadata &meta = plmov->getMovie()->getMetadata();

        shader->useProgram("viewport");

        glm::mat4 trf = camera.getViewMatrix();
        trf = glm::scale(trf, {1.0f, float(meta.SizeY) / float(meta.SizeX), 1.0f});
        shader->setMatrix4f("u_transform", glm::value_ptr(trf));

        shader->setInteger("u_nChannels", meta.SizeC);

        std::array<float, 15> vColor = {0.0f};
        for (uint32_t ch = 0; ch < meta.SizeC; ch++)
        {
            texture->bind(ch, ch);
            memcpy(vColor.data() + 3 * ch, &(plmov->getColor(ch))[0], 3 * sizeof(float));
        }
        shader->setVec3fArray("u_color", vColor.data(), 5);

        int vTex[5] = {0, 1, 2, 3, 4};
        shader->setIntArray("u_texture", vTex, 5);

        quad->draw();

    } // movie-plugin

    viewBuf->unbind();

    std::cout << deltaTime << std::endl;

} // function

void GPTool::ImGuiLayer(void)
{
    manager->showHeader();
    manager->showProperties();

    ///////////////////////////////////////////////////////

    ImGui::Begin("Viewport", NULL, ImGuiWindowFlags_NoTitleBar);

    // Check if it needs to resize
    ImVec2 port = ImGui::GetContentRegionAvail();
    ImGui::Image((void *)(uintptr_t)viewBuf->getID(), port);

    glm::vec2 view = viewBuf->getSize();
    if (port.x != view.x || port.y != view.y)
    {
        viewBuf = std::make_unique<Framebuffer>(port.x, port.y);
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

    std::thread([&](void) -> void {
        MoviePlugin *movie = new MoviePlugin(path, this);
        if (movie->successful())
        {
            texture = nullptr;
            camera.reset();

            manager = std::make_unique<PluginManager>(&fonts);
            manager->addPlugin("MOVIE", movie);
            manager->setActive("MOVIE");

            // TODO: add all the other plugins
        }
        else
            delete movie;
    }).detach();
}

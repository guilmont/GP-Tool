#include "gptool.h"

GPTool::GPTool(void)
{
    initialize("GShader", 1200, 800);
    shader = std::make_unique<Shader>();
    quad = std::make_unique<Quad>();

    fBuffer["viewport"] = std::make_unique<Framebuffer>(1200, 800);
} // function

void GPTool::onUserUpdate(float deltaTime)
{

    if (keyboard.get(Key::LEFT_CONTROL) == Event::PRESS && keyboard.get('I') == Event::RELEASE)
        mbox.create<Message::Info>("Double key");

    if (keyboard.get('W') == Event::RELEASE)
        mbox.create<Message::Warn>("opa");

    if (keyboard.get('E') == Event::RELEASE)
        mbox.create<Message::Error>("aiai");

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

    ImGui::Begin("Plugins");

    // float width = ImGui::GetContentRegionAvailWidth();
    // ImVec2 buttonSize{width, 30};
    // ImVec4
    //     chosenColor{0.1, 0.6, 0.1, 1.0},
    //     hoverColor{0.2, 0.7, 0.2, 1.0};

    // auto coloredButton = [&](const String &label) {
    //     bool check = data->activePlugin.compare(label) == 0;

    //     if (check)
    //     {
    //         ImGui::PushStyleColor(ImGuiCol_Button, chosenColor);
    //         ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
    //         ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
    //     }

    //     if (ImGui::Button(label.c_str(), buttonSize))
    //         data->activePlugin = label;

    //     if (check)
    //         ImGui::PopStyleColor(3);
    // };

    // ui.fonts.push("bold");

    // for (auto &[name, plugin] : data->mPlugin)
    //     if (plugin.active)
    //         coloredButton(name);

    // ui.fonts.pop();

    ImGui::End();

    // ///////////////////////////////////////////////////////

    ImGui::Begin("Properties");
    // data->mPlugin[data->activePlugin].callback(&ui);
    ImGui::End();

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
        {
            dialog.createDialog(GDialog::OPEN, "Choose TIF file...", {".tif", ".ome.tif"});
            // dialog->callback(openMovie, &ui);
        }

        // bool enable = data.movie ? true : false;
        bool enable = false;
        if (ImGui::MenuItem("Load trajectories...", NULL, nullptr, enable))
        {
            // ui.mWindows["openTraj"]->show();
        }

        if (ImGui::MenuItem("Save as ...", NULL, nullptr, enable))
        {
            // FileDialog *dialog = reinterpret_cast<FileDialog *>(ui.mWindows["dialog"].get());
            // dialog->createDialog(DIALOG_SAVE, "Save as...", {".hdf", ".json"});

            // void (*func)(const String &, UI *) = &saveData;
            // dialog->callback(func, &ui);
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
    if (mouse.get(Mouse::LEFT) == Event::PRESS)
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
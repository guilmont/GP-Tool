#include "filterPlugin.h"

FilterPlugin::FilterPlugin(GPT::Movie* movie, GPTool* ptr) : mov(movie), tool(ptr) {}

FilterPlugin::~FilterPlugin(void)
{
	for (auto [name, filter] : vFilters)
		delete filter;
}

void FilterPlugin::showProperties(void)
{
	ImGui::Begin("Properties");

    {
        float width = 100.0f * GRender::DPI_FACTOR;
        float pos = 0.4f * ImGui::GetContentRegionAvailWidth();

        // Chossing which channel to apply filters
        int32_t SC = int32_t(mov->getMetadata().SizeC) - 1;
        static int32_t locCH = 0;

        ImGui::Text("Channel:");
        ImGui::SameLine();
        ImGui::SetCursorPosX(pos);
        ImGui::SetNextItemWidth(width);
        ImGui::DragInt("##channel", &locCH, 0.5f, 0, SC);
        ImGui::SameLine();
        if (ImGui::Button("Set"))
        {
            currentCH = locCH;
            loadImages();
        }

        // Setting the number of threads to be used during filtering
        ImGui::Text(" Number of threads:");
        ImGui::SameLine();
        ImGui::SetCursorPosX(pos);
        ImGui::SetNextItemWidth(width);
        ImGui::DragInt("##threads", &numThreads, 0.5f, 1, std::thread::hardware_concurrency());

    }

    ImGui::Dummy({ 0, 5.0f * GRender::DPI_FACTOR });

    // Choosing which filters to use
	std::vector<const char*> filterNames = { "Contrast", "Median", "CLAHE", "SVD" };

    static uint64_t currentID = 0;
    ImGui::Text("Choose filter:");
    if (ImGui::BeginCombo("##addCombo", filterNames[currentID]))
    {
        for (uint64_t k = 0; k < filterNames.size(); k++)
        {
            const bool is_selected = (currentID == k);
            if (ImGui::Selectable(filterNames[k], is_selected))
                currentID = k;

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();

    if (ImGui::Button("Add"))
    {
        std::string name = std::to_string(filterCounter++) + "_" + std::string(filterNames[currentID]);

        switch (currentID)
        {
        case 0:
            vFilters[name] = new GPT::Filter::Contrast();
            break;
        case 1:
            vFilters[name] = new GPT::Filter::Median();
            break;
        case 2:
            vFilters[name] = new GPT::Filter::CLAHE();
            break;
        case 3:
            vFilters[name] = new GPT::Filter::SVD();
            break;
        }
    }

    ImGui::Dummy({ 0.0f, 5.0f * GRender::DPI_FACTOR });

    ImGui::BeginChild("filterChildren", { 0.0f, 312.0f * GRender::DPI_FACTOR }, true);
     
    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_None;
    nodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
    nodeFlags |= ImGuiTreeNodeFlags_Framed;
    nodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;
    nodeFlags |= ImGuiTreeNodeFlags_AllowItemOverlap;

    float width = ImGui::GetContentRegionAvailWidth();
    float height = ImGui::GetTextLineHeight();
    std::string toRemove = "";

    // Laying out filters for configuration
    for (auto [name, ptr] : vFilters)
    {
        // As every filter has an unique identifier, we will push this ID for every tree node
        ImGui::PushID(name.c_str());

        bool openTree = ImGui::TreeNodeEx(name.c_str(), nodeFlags);
        ImGui::SameLine(0.85f*width);

        if (ImGui::Button("Remove"))
            toRemove = name;

        if (openTree)
        {
            if (name.find("Contrast") != std::string::npos)
                displayContrast(reinterpret_cast<GPT::Filter::Contrast*>(ptr));

            else if (name.find("Median") != std::string::npos)
                displayMedian(reinterpret_cast<GPT::Filter::Median*>(ptr));

            else if (name.find("CLAHE") != std::string::npos)
                displayCLAHE(reinterpret_cast<GPT::Filter::CLAHE*>(ptr));

            else if (name.find("SVD") != std::string::npos)
                displaySVD(reinterpret_cast<GPT::Filter::SVD*>(ptr));
       
            ImGui::Dummy({ 0, 5.0f * GRender::DPI_FACTOR });

            ImGui::TreePop();
        }

        ImGui::PopID();
    }
    
    if (toRemove.size() > 0)
    {
        delete vFilters[toRemove]; // Removing from memory to avoid memory leak
        vFilters.erase(toRemove);  // erasing from hash table

        // Let's also reset the filter's counter
        if (vFilters.empty())
            filterCounter = 0;
    }


    ImGui::EndChild();

    {
        float pos = 0.8f * ImGui::GetContentRegionAvailWidth();

        if (ImGui::Button("Apply filters"))
        {
            cancel = false;
            prog = tool->mailbox.createProgress("Applying filters...", [](void* ptr) {
                FilterPlugin* fil = reinterpret_cast<FilterPlugin*>(ptr);
                fil->cancel = true;
                fil->loadImages();
                }, this);

            std::thread(&FilterPlugin::applyFilters, this).detach();
        }

        ImGui::SameLine();
        if (ImGui::Button("Reset"))
            std::thread(&FilterPlugin::loadImages, this).detach();


        ImGui::SameLine();

        ImGui::SetCursorPosX(pos);
        if (ImGui::Button("View"))
            viewWindow = true;


        ImGui::SameLine();
        if (ImGui::Button("Save"))
            tool->dialog.createDialog(GDialog::SAVE, "Save TIF file...", { "tif", "ome.tif" }, this,
                [](const fs::path& path, void* ptr) -> void { std::thread(&FilterPlugin::saveImages, reinterpret_cast<FilterPlugin*>(ptr), path).detach(); });

    }

    ImGui::End();

}

void FilterPlugin::showWindows(void)
{
    if (!viewWindow)
        return;

    ImGui::Begin("Filtered Images", &viewWindow);

    // Check if it needs to resize
    ImVec2 port = ImGui::GetContentRegionAvail();
    port.y -= 3.0f*ImGui::GetTextLineHeight();

    ImGui::Image((void*)(uintptr_t)fBuffer->getID(), port);
    viewHover = ImGui::IsItemHovered();

    glm::vec2 view = fBuffer->getSize();
    if (port.x != view.x || port.y != view.y)
    {
        fBuffer = std::make_unique<GRender::Framebuffer>(uint32_t(port.x), uint32_t(port.y));
        camera.setAspectRatio(port.x / port.y);
    }

    // Checking if anchoring position changed
    ImVec2 pos = ImGui::GetItemRectMin();
    fBuffer->setPosition(pos.x - tool->window.position.x, pos.y - tool->window.position.y);


    ImGui::Text("Channel: %d", currentCH);
    ImGui::Text("Frame:");
    ImGui::SameLine();
    if (ImGui::DragInt("##frame", &currentFR, 1.0f, 0, int32_t(vImages.size()) - 1))
        updateTexture = true;

    ImGui::End();
}

void FilterPlugin::update(float deltaTime)
{
    if (first)
    {
        first = false;

        // Creating local framebuffer and quad API
        fBuffer = std::make_unique<GRender::Framebuffer>(1, 1);
        quad = std::make_unique<GRender::Quad>(1);

        // Creating texture for plugin
        const MatXd& mat = mov->getImage(0, 0);
        uint32_t width = uint32_t(mat.cols()), height = uint32_t(mat.rows());
        tool->texture.createFloat("denoise", width, height);
    }

    // No point in updating the window if it is not seen
    if (!viewWindow)
        return;
     
    // If we are going to use this plugin for real, we load the images
    if (vImages.empty())
        loadImages();

    if (viewHover)
    {
        ImGuiIO& io = ImGui::GetIO();

         // camera controls
        if (ImGui::IsMouseDown(GMouse::LEFT))
        {
            glm::vec2 dr = { io.MouseDelta.x * deltaTime, io.MouseDelta.y * deltaTime };
            camera.moveHorizontal(dr.x);
            camera.moveVertical(dr.y);
        }

        if (io.MouseWheel > 0.0f)
            camera.moveFront(deltaTime);

        else if (io.MouseWheel < 0.0f)
            camera.moveBack(deltaTime);


        if (ImGui::IsKeyPressed(GKey::RIGHT))
        {
            currentFR += (currentFR + 1 == int32_t(vImages.size())) ? 0 : 1;
            updateTexture = true;

        }
        else if (ImGui::IsKeyPressed(GKey::LEFT))
        {
            currentFR -= (currentFR == 0) ? 0 : 1;
            updateTexture = true;
        }
    }


    // Do we need to update GPU's texture
    if (updateTexture)
    {
        updateTexture = false;

        MatXf mat = vImages[currentFR].cast<float>();
        tool->texture.updateFloat("denoise", mat.data());
    }

    // Updating frame buffer
    glm::mat4 trf = camera.getViewMatrix();
    auto info = tool->texture.getSpecification("denoise");

    float size[2] = { float(info.width), float(info.height) };
    trf = glm::scale(trf, { 1.0f, size[1] / size[0], 1.0f });

    fBuffer->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.6f, 0.6f, 0.6f, 1.0f);

     tool->shader.useProgram("denoise");
     tool->shader.setMatrix4f("u_transform", glm::value_ptr(trf));

     tool->texture.bind("denoise", 0);
     tool->shader.setInteger("u_texture", 0);

     quad->draw({ 0.0f,0.0f,0.0f }, { 1.0f, 1.0f }, 0.0f, 0.0f);
     quad->submit();

    fBuffer->unbind();

}

////////////////////////////////////////////////////////////////////////////
// UTILITY FUNCTIONS

void FilterPlugin::loadImages(void)
{
    uint64_t ST = mov->getMetadata().SizeT;
    vImages.resize(ST);

    for (uint64_t k = 0; k < ST; k++)
    {
        vImages[k] = mov->getImage(currentCH, k);

        // This images are not in the 0-1 interval. Let's set this interval using some sort of auto-contrast
        double
            bot = 0.8 * vImages[k].minCoeff(),
            top = 1.2 * vImages[k].maxCoeff();

        vImages[k].array() = (vImages[k].array() - bot) / (top - bot);
    }

    // We need to update the texture we are seeing
    updateTexture = true;
}

void FilterPlugin::applyFilters(void)
{
    // To avoid loading things we don't need, we only load images for display or running the filters
    if (vImages.empty())
        loadImages();


    int64_t 
        total = vFilters.size() * vImages.size(),
        counter = 0;

    // Let's reset the images so we don't need to do it manually every time
    loadImages();

    for (auto [name, ptr] : vFilters)
    {
        if (name.find("SVD") != std::string::npos) // Our special boy 
        {
            GPT::Filter::SVD* svd = reinterpret_cast<GPT::Filter::SVD*>(ptr);

            svd->importImages(vImages);
            svd->run(cancel);
            svd->updateImages(vImages);

            counter += vImages.size();
            prog->progress = float(counter) / float(total);
        }
        else
        {
            // Applying filter on all images
            auto parallelFunction = [&](int32_t tid, GPT::Filter::Filter* filter) -> void {

                for (int32_t k = tid; k < vImages.size(); k += numThreads)
                {
                    if (cancel)
                        return;

                    filter->apply(vImages[k]);

                    if (tid == 0)
                    {
                        counter += numThreads;
                        prog->progress = float(counter) / float(total);
                    }
                }
            };

            // Splitting in threads
            std::vector<std::thread> vThr(numThreads);
            for (int32_t k = 0; k < numThreads; k++)
                vThr[k] = std::thread(parallelFunction, k, ptr);

            for (std::thread& thr : vThr)
                thr.join();
        }

        // No need to continue with other filters
        if (cancel)
            break;
    }

    // Wrapping up function
    if (cancel)
    {
        prog->is_read = true;
        tool->mailbox.createInfo("Filters execution cancelled");
    }
    else
    {
        updateTexture = true;
        prog->progress = 1.0f;
        tool->mailbox.createInfo("Filters execution completed");
    }
    
}

void FilterPlugin::saveImages(const fs::path& address)
{
    // Converting images into 16-bit
    std::vector<Image<uint16_t>> vec(vImages.size());
    for (uint64_t k = 0; k < vImages.size(); k++)
        vec[k] = (65535.0 * vImages[k]).cast<uint16_t>();

    // Saving to input path
    GPT::Tiffer::Write wrt(vec);
    wrt.save(address);

    tool->mailbox.createInfo("Diltered images saved to " + address.string());
}

////////////////////////////////////////////////////////////////////////////
// DISPLAY FUNCTIONS

// Display configuration settings for filters
void FilterPlugin::displayContrast(GPT::Filter::Contrast* ptr)
{
    float low = float(ptr->low);
    float high = float(ptr->high);

    ImGui::Text("Low:");
    ImGui::SameLine();
    if (ImGui::DragFloat("##low", &low, 0.1f, 0.0f, high, "%.3f"))
        ptr->low = double(low);

    ImGui::Text("High:");
    ImGui::SameLine();
    if (ImGui::DragFloat("##high", &high, 0.1f, low, 1.0f, "%.3f"))
        ptr->high = double(high);

}

void FilterPlugin::displayMedian(GPT::Filter::Median* ptr)
{
    int32_t sx = int32_t(ptr->sizeX);
    int32_t sy = int32_t(ptr->sizeY);

    ImGui::Text("Size X:");
    ImGui::SameLine();
    if (ImGui::DragInt("##sizeX", &sx, 0.5f, 3, 64, "%.3f"))
        ptr->sizeX = int64_t(sx);

    ImGui::Text("Size Y:");
    ImGui::SameLine();
    if (ImGui::DragInt("##sizeY", &sy, 0.5f, 3, 64, "%.3f"))
        ptr->sizeY = int64_t(sy);

}

void FilterPlugin::displayCLAHE(GPT::Filter::CLAHE* ptr)
{
    int32_t
        TX = int32_t(ptr->tileSizeX),
        TY = int32_t(ptr->tileSizeY);

    float 
        clip = float(ptr->clipLimit);

    ImGui::Text("Tile X:");
    ImGui::SameLine();
    if (ImGui::DragInt("##tileX", &TX, 0.5f, 8, 256))
        ptr->tileSizeX = int64_t(TX);

    ImGui::Text("Tile Y:");
    ImGui::SameLine();
    if (ImGui::DragInt("##tileY", &TY, 0.5f, 8, 256))
        ptr->tileSizeY = int64_t(TY);


    ImGui::Text("Clip limit:");
    ImGui::SameLine();
    if (ImGui::DragFloat("##clip", &clip, 0.1f, 0.1f, 10.0f, "%.3f"))
        ptr->clipLimit = double(clip);

}

void FilterPlugin::displaySVD(GPT::Filter::SVD* ptr)
{
    int32_t
        ST = int32_t(mov->getMetadata().SizeT),
        slice = int32_t(ptr->slice),
        rank = int32_t(ptr->rank);

    ImGui::Text("Slice:");
    ImGui::SameLine();
    if (ImGui::DragInt("##slice", &slice, 0.5f, 1, ST))
        ptr->slice = int64_t(slice);

    ImGui::Text("Rank:");
    ImGui::SameLine();
    if (ImGui::DragInt("##rank", &rank, 0.5f, 1, ST))
        ptr->rank = int64_t(rank);

}



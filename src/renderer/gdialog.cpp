#include "renderer/gdialog.h"

int inputTextCallback(ImGuiInputTextCallbackData *data)
{
    GDialog *diag = (GDialog *)data->UserData;
    if (diag->lFolders.size() > 0)
    {
        diag->main_path += "/" + diag->lFolders.front();

        String diff = diag->main_path.substr(data->BufTextLen) + "/";
        data->InsertChars(data->BufTextLen, diff.c_str());
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS

void GDialog::createDialog(uint32_t type, const String &title, const std::list<String> &ext,
                           void *data, void (*callback)(void *))
{
    active = true;
    this->title = std::move(title);
    currentExt = ext.front();
    this->lExtension = std::move(ext);

    switch (type)
    {
    case GDialog::OPEN:
        dialog_function = &GDialog::openDialog;
        break;
    case GDialog::SAVE:
        dialog_function = &GDialog::saveDialog;
        break;
    default:
        std::cerr << "ERROR: Unknown dialog type" << std::endl;
        exit(-1);
        break;
    }

    this->callback = callback;
    this->callback_data = data;

} // createDialog

void GDialog::showDialog(void)
{
    if (!active)
        return;

    ImGui::Begin(title.c_str(), &active);
    bool status = (this->*dialog_function)();
    ImGui::End();

    if (existPopup)
        status = fileExistsPopup();

    if (status && callback)
        callback(callback_data);
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

bool GDialog::openDialog(void)
{
    bool status = false;

    // to avoid path problems later
    if (main_path[main_path.size() - 1] == '/')
        main_path.resize(main_path.size() - 1);

    ImGui::Text("Input path to file:");

    ImGui::SameLine();
    if (ImGui::Button("Back"))
    {
        size_t pos = main_path.find_last_of('/');
        main_path = main_path.substr(0, pos);
    }

    static char loc[1024] = {0};
    sprintf_s(loc, "%s", main_path.c_str());

    float width = ImGui::GetContentRegionAvailWidth();

    ImGui::PushItemWidth(width);
    ImGui::InputText("##MainAdress", loc, 1024,
                     ImGuiInputTextFlags_CallbackCompletion, inputTextCallback, this);

    ImGui::PopItemWidth();

    String prov(loc);
    if (prov.size() == 0)
        prov = main_path;

    bool toOpen = systemDisplay(prov);

    ImGui::Text("Extensions:");
    ImGui::SameLine();

    ImGui::PushItemWidth(100);
    if (ImGui::BeginCombo("##combo", currentExt.c_str()))
    {
        for (const String &ext : lExtension)
        {
            bool check = currentExt.compare(ext) == 0;
            if (ImGui::Selectable(ext.c_str(), &check))
            {
                currentExt = ext;
                ImGui::SetItemDefaultFocus();
            } // if-selected
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Separator();

    ImGui::Spacing();

    if (ImGui::Button("Open") | toOpen)
    {
        filepath = main_path + "/" + selected;
        if (std::filesystem::is_regular_file(filepath))
        {
            filename = selected;
            status = true;
            active = false;
        }
        else
        {
            main_path += "/" + selected;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        active = false;

    return status;

} // openDialog

bool GDialog::saveDialog(void)
{
    bool status = false;

    // to avoid path problems later
    if (main_path[main_path.size() - 1] == '/')
        main_path.resize(main_path.size() - 1);

    ImGui::Text("Output path:");

    ImGui::SameLine();
    if (ImGui::Button("Back"))
    {
        size_t pos = main_path.find_last_of('/');
        main_path = main_path.substr(0, pos);
    }

    static char loc[1024];
    sprintf_s(loc, "%s", main_path.c_str());

    float width = ImGui::GetContentRegionAvailWidth();

    ImGui::PushItemWidth(width);
    ImGui::InputText("##MainAdress", loc, 1024,
                     ImGuiInputTextFlags_CallbackCompletion, inputTextCallback, this);

    ImGui::PopItemWidth();

    String prov(loc);
    if (prov.size() == 0)
        prov = main_path;

    bool toOpen = systemDisplay(prov);

    ///////////////////////////////////////////////////////

    ImGui::Text("Filename");
    ImGui::SameLine();

    char buf[64] = {0};
    if (filename.size() > 0)
    {
        size_t pos = filename.find_last_of('.');
        memcpy(buf, filename.c_str(), pos);
        currentExt = filename.substr(pos);
    }

    ImGui::PushItemWidth(0.333f * width);
    if (ImGui::InputText("##inp", buf, 64))
        filename = std::string(buf) + currentExt;
    ImGui::PopItemWidth();

    ImGui::SameLine();

    ImGui::PushItemWidth(100);
    if (ImGui::BeginCombo("##combo", currentExt.c_str()))
    {
        for (const String &ext : lExtension)
        {
            bool check = currentExt.compare(ext) == 0;
            if (ImGui::Selectable(ext.c_str(), &check))
            {
                currentExt = ext;
                filename = std::string(buf) + currentExt;
                ImGui::SetItemDefaultFocus();
            } // if-selected
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Separator();

    ///////////////////////////////////////////////////////
    ImGui::Spacing();

    if (ImGui::Button("Save") | toOpen)
    {
        // NEED to perform some checking
        filepath = main_path + "/" + selected;
        if (std::filesystem::is_regular_file(filepath))
        {
            filename = selected;
            existPopup = true;
        }

        else
        {
            status = true;
            active = false;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        active = false;

    return status;
}

///////////////////////////////////////////////////////////////////////////////

void GDialog::systemLoop(void)
{
    for (const auto &entry : std::filesystem::directory_iterator(main_path))
    {
        if (std::filesystem::is_directory(entry))
        {
            String arq = entry.path().string();

#ifdef _WIN32
            size_t pos = arq.find_last_of("\\");
#elif
            size_t pos = arq.find_last_of("/");
#endif

            arq = arq.substr(pos + 1);
            if (arq[0] == '.')
                continue;

            lFolders.push_back(arq);

        } // loop-diretory and extensions

        else if (std::filesystem::is_regular_file(entry))
        {
            String arq = entry.path().string();

#ifdef _WIN32
            size_t pos = arq.find_last_of("\\");
#elif
            size_t pos = arq.find_last_of("/");
#endif

            arq = arq.substr(pos + 1);

            // Excluding hidden files
            if (arq[0] == '.')
                continue;

            if (arq.find(currentExt) != String::npos)
                lFiles.push_back(arq);
        }
    } // loop-exists

} // loop-directories

bool GDialog::systemDisplay(const String &url)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.02f, 0.02f, 0.02f, 1.0f});

    lFolders.clear();
    lFiles.clear();

    if (url.size() == 0)
    {
        // nothing to do
    }
    else if (std::filesystem::is_directory(url))
    {
        main_path = std::move(url);
        systemLoop();
    }

    else if (std::filesystem::is_regular_file(url))
    {
        size_t pos = url.find_last_of('/');
        main_path = url.substr(0, pos);

        for (const String &ext : lExtension)
            if (url.find(ext) != String::npos)
            {
                selected = url.substr(pos + 1);
                break;
            }
    }

    else // It is probably a incomplete path
    {
        size_t pos = url.find_last_of('/');
        pos = pos == 0 ? 1 : pos; // Protection for linux and mac

        main_path = url.substr(0, pos);

        systemLoop();

        pos = pos == 1 ? pos : pos + 1;
        String diff = url.substr(pos);

        lFolders.remove_if([&](const String &name) -> bool {
            return name.find(diff) == String::npos;
        });

        lFiles.remove_if([&](const String &name) -> bool {
            return name.find(diff) == String::npos;
        });
    }

    ///////////////////////////////////////////////////
    // Displaying folders and files

    float width = ImGui::GetContentRegionAvailWidth();
    ImGui::BeginChild("child_2", {width, 256 * DPI_FACTOR}, true);

    lFolders.sort();
    for (auto &folder : lFolders)
    {
        bool check = selected.compare(folder) == 0;
        if (check)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, {0.6f, 0.1f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.6f, 0.1f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.7f, 0.2f, 0.2f, 1.0f});
        }

        if (ImGui::Selectable(folder.c_str(), true, ImGuiSelectableFlags_AllowDoubleClick))
        {
            selected = folder;

            if (ImGui::IsMouseDoubleClicked(0))
                main_path += "/" + folder;
        }

        if (check)
            ImGui::PopStyleColor(3);

        ImGui::Spacing();

    } // loop-folders

    bool toOpen = false;
    lFiles.sort();
    for (auto &arq : lFiles)
    {
        bool check = selected.compare(arq) == 0;

        if (check)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, {0.1f, 0.6f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.1f, 0.6f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.2f, 0.7f, 0.2f, 1.0f});
        }

        if (ImGui::Selectable(arq.c_str(), check, ImGuiSelectableFlags_AllowDoubleClick))
        {
            selected = arq;

            if (ImGui::IsMouseDoubleClicked(0))
            {
                toOpen = true;
                filename = arq;
            }
        }

        if (check)
            ImGui::PopStyleColor(3);
    }

    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::PopStyleColor();

    return toOpen;
} // systemDisplay

bool GDialog::fileExistsPopup(void)
{
    bool status = false;
    ImGui::OpenPopup("File exists");

    if (ImGui::BeginPopupModal("File exists"))
    {
        ImGui::Text("'%s' already exists. Replace?", filename.c_str());

        if (ImGui::Button("Yes"))
        {
            existPopup = active = false;
            status = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No"))
        {
            existPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } // popup-file-exists

    return status;
}
#include "fileDialog.h"

int inputTextCallback(ImGuiInputTextCallbackData *data)
{
    FileDialog *diag = (FileDialog *)data->UserData;
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

FileDialog::FileDialog(void)
{
    this->main_path = std::filesystem::current_path();
} // constructor

void FileDialog::createDialog(uint32_t type, const String &title,
                              const std::list<String> &extension)
{
    this->dialogID = type;
    this->title = std::move(title);
    this->lExtension = std::move(extension);
    this->currentExt = lExtension.front();
    this->filename = "";
    this->foldername = "";

    this->show();
} // setExtensions

void FileDialog::display(void)
{
    if (dialogID == DIALOG_OPEN)
        openDialog();
    else
        saveDialog();

    if (state == DIALOG_SUCESS)
    {
        state = DIALOG_CALL;
        this->hide();
    }
    else if (state == DIALOG_CLOSE)
        this->hide();

} // showDialog

///////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

void FileDialog::systemLoop(void)
{
    for (const auto &entry : std::filesystem::directory_iterator(main_path))
    {
        if (std::filesystem::is_directory(entry))
        {
            String arq = entry.path();
            size_t pos = arq.find_last_of("/");
            arq = arq.substr(pos + 1);
            if (arq[0] == '.')
                continue;

            lFolders.push_back(arq);

        } // loop-diretory and extensions

        else if (std::filesystem::is_regular_file(entry))
        {
            String arq = entry.path();

            size_t pos = arq.find_last_of("/");
            arq = arq.substr(pos + 1);

            // Excluding hidden files
            if (arq[0] == '.')
                continue;

            for (const String &ext : lExtension)
                if (arq.find(ext) != String::npos)
                {
                    lFiles.push_back(arq);
                    break;
                }
        }
    } // loop-exists

} // loop-directories

bool FileDialog::systemDisplay(const String &newMain)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.02, 0.02, 0.02, 1.0});

    lFolders.clear();
    lFiles.clear();

    if (std::filesystem::is_directory(newMain))
    {
        main_path = std::move(newMain);
        systemLoop();
    }

    else if (std::filesystem::is_regular_file(newMain))
    {
        for (const String &ext : lExtension)
            if (newMain.find(ext) != String::npos)
            {
                size_t pos = newMain.find_last_of('/');
                main_path = newMain.substr(0, pos);
                filename = newMain.substr(pos + 1);
                break;
            }
    }

    else // It is probably a incomplete path
    {
        if (newMain.size() < main_path.size())
        {
            size_t pos = newMain.find_last_of('/');
            main_path = newMain.substr(0, pos);
        }

        systemLoop();

        String diff = newMain.substr(main_path.size() + 1);

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
    ImGui::BeginChild("child_2", {width, 256}, true);

    lFolders.sort();
    for (auto &folder : lFolders)
    {
        bool check = folder.compare(foldername) == 0;
        if (check)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, {0.6, 0.1, 0.1, 1.0});
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.6, 0.1, 0.1, 1.0});
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.7, 0.2, 0.2, 1.0});
        }

        ImGui::Selectable(folder.c_str(), true);

        if (check)
            ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
                foldername = folder;

            if (ImGui::IsMouseDoubleClicked(0))
            {
                if (main_path[main_path.size() - 1] == '/')
                    main_path += folder;
                else
                    main_path += "/" + folder;
            }
        }

        ImGui::Spacing();

    } // loop-folders

    bool toOpen = false;
    lFiles.sort();
    for (auto &arq : lFiles)
    {
        bool check = filename.compare(arq) == 0;

        if (check)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, {0.1, 0.6, 0.1, 1.0});
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.1, 0.6, 0.1, 1.0});
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.2, 0.7, 0.2, 1.0});
        }

        ImGui::Selectable(arq.c_str(), check);

        if (ImGui::IsItemHovered())
        {
            if (ImGui::IsMouseClicked(0))
                filename = arq;

            if (ImGui::IsMouseDoubleClicked(0))
                toOpen = true;
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

void FileDialog::openDialog(void)
{
    state = DIALOG_READY;

    // to avoid path problems later
    if (main_path[main_path.size() - 1] == '/')
        main_path.resize(main_path.size() - 1);

    ImGui::SetNextWindowSize({700, 400});
    ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Text("Input path to file:");

    ImGui::SameLine();
    if (ImGui::Button("Back"))
    {
        size_t pos = main_path.find_last_of('/');
        main_path = main_path.substr(0, pos);
    }

    static char loc[1024];
    sprintf(loc, "%s", main_path.c_str());

    float width = ImGui::GetContentRegionAvailWidth();

    ImGui::PushItemWidth(width);
    ImGui::InputText("##MainAdress", loc, 1024, ImGuiInputTextFlags_CallbackCompletion, inputTextCallback, this);

    ImGui::PopItemWidth();

    bool toOpen = systemDisplay(loc);

    if (ImGui::Button("Open") | toOpen)
    {
        path2File = main_path + "/" + filename;
        if (std::filesystem::is_regular_file(path2File))
            state = DIALOG_SUCESS;
        else
        {
            String newFolder = main_path + "/" + foldername;
            if (std::filesystem::is_directory(newFolder))
                main_path = newFolder;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        state = DIALOG_CLOSE;

    ImGui::End();

} // openDialog

void FileDialog::saveDialog(void)
{
    state = DIALOG_READY;

    // to avoid path problems later
    if (main_path[main_path.size() - 1] == '/')
        main_path.resize(main_path.size() - 1);

    ImGui::SetNextWindowSize({720, 435});
    ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoResize);
    ImGui::Text("Output path:");

    ImGui::SameLine();
    if (ImGui::Button("Back"))
    {
        size_t pos = main_path.find_last_of('/');
        main_path = main_path.substr(0, pos);
    }

    static char loc[1024];
    sprintf(loc, "%s", main_path.c_str());

    float width = ImGui::GetContentRegionAvailWidth();

    ImGui::PushItemWidth(width);
    ImGui::InputText("##MainAdress", loc, 1024, ImGuiInputTextFlags_CallbackCompletion, inputTextCallback, this);

    ImGui::PopItemWidth();

    bool toOpen = systemDisplay(loc);

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

    ImGui::PushItemWidth(0.333 * width);
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
        path2File = main_path + "/" + filename;
        if (std::filesystem::is_regular_file(path2File))
            existPopup = true;

        else
            state = DIALOG_SUCESS;
    }

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        state = DIALOG_CLOSE;

    ImGui::End();

    if (existPopup)
        fileExistsPopup();

} // openDialog

void FileDialog::fileExistsPopup(void)
{
    ImGui::OpenPopup("File exists");

    if (ImGui::BeginPopupModal("File exists"))
    {
        ImGui::Text("'%s' already exists. Replace?", filename.c_str());

        if (ImGui::Button("Yes"))
        {
            existPopup = false;
            state = DIALOG_SUCESS;
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
}
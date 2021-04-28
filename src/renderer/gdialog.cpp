#include "renderer/gdialog.h"

#include <cstdlib>
#include <iostream>

#ifdef WIN32
static constexpr char slash = '\\';
#else
static constexpr char slash = '/';
#endif

int inputTextCallback(ImGuiInputTextCallbackData *data)
{
    GDialog *diag = (GDialog *)data->UserData;

    if (diag->probable.size() > 0)
    {
        size_t pos = diag->probable.size() - data->BufTextLen;
        std::string diff = diag->probable.substr(data->BufTextLen);
        data->InsertChars(data->BufTextLen, diff.c_str());
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS

GDialog::GDialog(void)
{
#ifdef WIN32
    main_path = std::getenv("USERPROFILE");
#else
    main_path = std::getenv("HOME");
#endif
}

GDialog::~GDialog(void) {}

void GDialog::createDialog(uint32_t type, const std::string &title,
                           const std::list<std::string> &ext,
                           void *data, void (*callback)(const std::string &, void *))
{
    active = true;
    this->title = std::move(title);
    this->currentExt = ext.front();
    this->lExtension = std::move(ext);
    this->selected = "";
    this->filename = "";

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

    bool status = false;

    if (existPopup)
        status = fileExistsPopup();
    else
    {
        ImGui::Begin(title.c_str(), &active);
        status = (this->*dialog_function)();
        ImGui::End();
    }

    if (status)
        callback(main_path.string(), callback_data);
}

///////////////////////////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS

bool GDialog::openDialog(void)
{
    bool status = false;

    ImGui::Text("Input path to file:");

    ImGui::SameLine();
    if (ImGui::Button("Back"))
        main_path = main_path.parent_path();

    static char loc[512] = {0};
    sprintf(loc, "%s", main_path.string().c_str());

    float width = ImGui::GetContentRegionAvailWidth();

    ImGui::PushItemWidth(width);
    ImGui::InputText("##MainAdress", loc, 512,
                     ImGuiInputTextFlags_CallbackCompletion, inputTextCallback, this);

    ImGui::PopItemWidth();

    bool toOpen = systemDisplay(loc);

    ImGui::Text("Extensions:");
    ImGui::SameLine();

    ImGui::PushItemWidth(100);
    if (ImGui::BeginCombo("##combo", currentExt.c_str()))
    {
        for (const std::string &ext : lExtension)
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
        main_path += slash + selected;
        if (fs::is_regular_file(main_path))
        {
            status = true;
            active = false;
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

    ImGui::Text("Output path:");

    ImGui::SameLine();
    if (ImGui::Button("Back"))
        main_path = main_path.parent_path();

    char buf[512] = {0};
    sprintf(buf, "%s", main_path.string().c_str());

    float width = ImGui::GetContentRegionAvailWidth();

    ImGui::PushItemWidth(width);
    ImGui::InputText("##MainAdress", buf, 512,
                     ImGuiInputTextFlags_CallbackCompletion, inputTextCallback, this);

    ImGui::PopItemWidth();

    if (systemDisplay(buf))
    {
        fs::path loc(selected);
        filename = loc.stem().string();
        currentExt = loc.extension().string();
    }

    ///////////////////////////////////////////////////////

    ImGui::Text("Filename");
    ImGui::SameLine();

    memset(buf, 0, 512);
    sprintf(buf, "%s", filename.c_str());

    ImGui::PushItemWidth(0.333f * width);
    ImGui::InputText("##inp", buf, 512);

    filename = std::string(buf);
    size_t pos = filename.find(".");
    if (pos != std::string::npos)
    {
        std::string ext = filename.substr(pos);
        filename = filename.substr(0, pos);

        // Just a check to be sure that extension is correct
        for (auto &xt : lExtension)
            if (ext.compare(xt) == 0)
            {
                currentExt = xt;
                break;
            }
    }

    ImGui::PopItemWidth();

    ImGui::SameLine();

    ImGui::PushItemWidth(100);
    if (ImGui::BeginCombo("##combo", currentExt.c_str()))
    {
        for (const std::string &ext : lExtension)
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

    ///////////////////////////////////////////////////////
    ImGui::Spacing();

    if (ImGui::Button("Save") && filename.size() > 0)
    {
        // NEED to perform some checking
        main_path = fs::path(main_path.string() += slash + filename + currentExt);
        if (fs::is_regular_file(main_path))
            existPopup = true;
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

bool GDialog::systemDisplay(std::string url)
{
    std::string diff = "";
    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.02f, 0.02f, 0.02f, 1.0f});

#ifdef WIN32
    uint32_t size = 3; // C:\ or D:\ or ...
#else
    uint32_t size = 1; // linux and mac use only /
#endif

    if (url.size() < size)
    {
    }

    else if (std::filesystem::is_directory(url))
        main_path = fs::path(url);

    else if (std::filesystem::is_regular_file(url))
    {
        main_path = fs::path(url).parent_path();

        fs::path loc = fs::path(url).filename();
        for (const std::string &ext : lExtension)
            if (loc.string().find(ext) != std::string::npos)
            {
                selected = loc.string();
                currentExt = loc.extension().string();
                break;
            }
    }
    else
    {
        fs::path loc(url);
        main_path = loc.parent_path();
        diff = loc.filename().string();
    }

    ///////////////////////////////////////////////////
    // Displaying folders and files

    bool toOpen = false;
    float width = ImGui::GetContentRegionAvailWidth();
    ImGui::BeginChild("child_2", {width, 256 * DPI_FACTOR}, true);

    probable = "";
    for (const auto &entry : fs::directory_iterator(main_path))
    {

        const fs::path &loc = entry.path();

        if (loc.string().find(diff) == std::string::npos)
            continue;

        bool isDir = false, isFile = false;
        try
        {
            isDir = fs::is_directory(loc);
            isFile = fs::is_regular_file(loc);
        }
        catch (...)
        {
        }

        if (isDir)
        {
            const std::string &folder = loc.filename().string();
            if (folder[0] == '.') // hidden folder
                continue;

            if (probable.size() == 0)
                probable = loc.string();

            bool check = selected.compare(folder) == 0;
            if (check)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, {0.6f, 0.1f, 0.1f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.6f, 0.1f, 0.1f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.7f, 0.2f, 0.2f, 1.0f});
            }

            if (ImGui::Selectable(folder.c_str(), true,
                                  ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected = folder;

                if (ImGui::IsMouseDoubleClicked(0))
                    main_path = loc;
            }

            if (check)
                ImGui::PopStyleColor(3);

            ImGui::Spacing();
        }

        else if (isFile)
        {
            const std::string &arq = loc.filename().string();
            if ((arq[0] == '.') || (arq.find(currentExt) == std::string::npos))
                continue;

            bool check = selected.compare(arq) == 0;
            if (check)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, {0.1f, 0.6f, 0.1f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, {0.1f, 0.6f, 0.1f, 1.0f});
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, {0.2f, 0.7f, 0.2f, 1.0f});
            }

            if (ImGui::Selectable(arq.c_str(), check,
                                  ImGuiSelectableFlags_AllowDoubleClick))
            {
                selected = arq;
                if (ImGui::IsMouseDoubleClicked(0))
                    toOpen = true;
            }

            if (check)
                ImGui::PopStyleColor(3);
        }

    } // loop-directory

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
        const std::string &name = main_path.filename().string();
        ImGui::Text("'%s' already exists. Replace?", name.c_str());

        if (ImGui::Button("Yes"))
        {
            existPopup = active = false;
            status = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("No"))
        {
            main_path = main_path.parent_path();
            existPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    } // popup-file-exists

    return status;
}
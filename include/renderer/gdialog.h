#pragma once

#include <string>
#include <list>

#include <imgui.h>

#include <filesystem>
namespace fs = std::filesystem;

class GDialog
{
public:
    GDialog(void);
    ~GDialog(void);

    void createDialog(uint32_t type, const std::string &title,
                      const std::list<std::string> &ext,
                      void *data = nullptr,
                      void (*callback)(const std::string &, void *) = nullptr);
    void showDialog(void);

    // RETRIEVE DATA
    // const std::string &getFilename(void) const { return filename; }
    // const std::string &getFolder(void) const { return main_path; }
    const std::string &getPath(void) const { return main_path.string(); }

    enum : uint32_t
    {
        OPEN,
        SAVE,
    };

private:
    bool active = false;
    std::string title;

    fs::path main_path;

    std::string currentExt, selected, probable, filename;
    std::list<std::string> lExtension;

    bool (GDialog::*dialog_function)(void);

    // callback
    void *callback_data = nullptr;
    void (*callback)(const std::string &, void *) = nullptr;

    // DISPLAY DIALOGS
    bool openDialog(void);
    bool saveDialog(void);

    bool systemDisplay(const std::string &url);

    bool existPopup = false;
    bool fileExistsPopup(void);

    friend int inputTextCallback(ImGuiInputTextCallbackData *);
};

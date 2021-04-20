#pragma once

#include <filesystem>
#include <thread>
#include <mutex>
#include <list>

#include <imgui.h>

#include "config.h"

class GDialog
{
public:
    GDialog(void) = default;
    ~GDialog(void) = default;

    void createDialog(uint32_t type, const String &title, const std::list<String> &ext,
                      void *data = nullptr, void (*callback)(void *) = nullptr);
    void showDialog(void);

    // RETRIEVE DATA
    const String &getFilename(void) const { return filename; }
    const String &getFolder(void) const { return main_path; }
    const String &getPath(void) const { return filepath; }

    enum : uint32_t
    {
        OPEN,
        SAVE,
    };

private:
    bool active = false;

    String filename, filepath, main_path = HOME_DIR;
    std::list<String> lFolders, lFiles;

    String title, currentExt, selected;
    std::list<String> lExtension;

    bool (GDialog::*dialog_function)(void);

    // callback
    void *callback_data = nullptr;
    void (*callback)(void *) = nullptr;

    // DISPLAY DIALOGS
    bool openDialog(void);
    bool saveDialog(void);

    bool systemDisplay(const String &url);
    void systemLoop(void);

    bool existPopup = false;
    bool fileExistsPopup(void);

    friend int inputTextCallback(ImGuiInputTextCallbackData *);
};

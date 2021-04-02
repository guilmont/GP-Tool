#pragma once

#include <filesystem>
#include <thread>
#include <mutex>
#include <list>

#include <imgui.h>

#include <config.h>

class GDialog
{
public:
    GDialog(void) = default;
    ~GDialog(void) = default;

    bool active = false;

    void createDialog(uint32_t type, const String &title, const std::list<String> &ext,
                      void *data = nullptr, void (*callback)(void *) = nullptr);
    void showDialog(void);

    // RETRIEVE DATA
    const String &
    getFilename(void) const
    {
        return filename;
    }
    const String &getMainPath(void) const { return main_path; }
    const String &getPath(void) const { return path2File; }

    enum : uint32_t
    {
        OPEN,
        SAVE,
    };

private:
    String main_path = HOME_DIR;
    String foldername, filename;
    std::list<String> lFolders, lFiles;

    String path2File;

    String title, currentExt;
    std::list<String> lExtension;

    bool (GDialog::*dialog_function)(void);

    // callback
    void *callback_data = nullptr;
    void (*callback)(void *) = nullptr;

    // DISPLAY DIALOGS
    bool openDialog(void);
    bool saveDialog(void);

    bool systemDisplay(const String &newMain);
    void systemLoop(void);

    bool existPopup = false;
    bool fileExistsPopup(void);

    friend int inputTextCallback(ImGuiInputTextCallbackData *);
};

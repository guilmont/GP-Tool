#pragma once

#include <filesystem>
#include <thread>
#include <string>
#include <mutex>
#include <list>

#include "window.h"
#include <imgui.h>

using String = std::string;

enum : uint32_t
{
    DIALOG_OPEN,
    DIALOG_SAVE,
    DIALOG_READY,
    DIALOG_CLOSE,
    DIALOG_SUCESS,
    DIALOG_CALL,
};

class FileDialog : public Window
{
public:
    FileDialog(void);
    ~FileDialog(void) = default;

    // SETUP
    void createDialog(uint32_t type, const String &title, const std::list<String> &extensions);

    template <typename FUNC, typename ARG>
    void callback(FUNC func, ARG arg);

    void display(void) override;

    // RETRIEVE DATA
    const String &getFilename(void) const { return filename; }
    const String &getMainPath(void) const { return main_path; }
    const String &getPath2File(void) const { return path2File; }

private:
    String path2File;
    String main_path, foldername, filename;
    std::list<String> lFolders, lFiles, lExtension;

    uint32_t state;
    String title, currentExt;
    uint32_t dialogID;
    bool existPopup = false;

    // DISPLAY DIALOGS
    void openDialog(void);
    void saveDialog(void);

    bool systemDisplay(const String &newMain);
    void systemLoop(void);
    void fileExistsPopup(void);

    friend int inputTextCallback(ImGuiInputTextCallbackData *);
};

template <typename FUNC, typename ARG>
void FileDialog::callback(FUNC func, ARG arg)
{
    auto parallel = [&](FUNC func, ARG arg) -> void {
        while (state == DIALOG_READY)
            std::this_thread::sleep_for(std::chrono::milliseconds(30));

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        if (state == DIALOG_CALL)
        {
            func(path2File, arg);
            this->hide();
        }
    };

    state = DIALOG_READY;
    std::thread(parallel, func, arg).detach();
}
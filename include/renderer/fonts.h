#pragma once

#include <imgui.h>
#include <string>
#include <map>

using String = std::string;

class Fonts
{
public:
    void loadFont(const String &fontname, const String &path, float size);
    void setDefault(const String &name);

    void text(const String &txt, const String &type);

    void push(const String &fontname);
    void pop(void);

private:
    String assets;
    std::map<String, ImFont *> mFonts;
};

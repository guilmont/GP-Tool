#include "fonts.h"

#include <filesystem>

void Fonts::setDefault(const String &name)
{
    ImGui::GetIO().FontDefault = mFonts[name];
}

void Fonts::loadFont(const String &fontname, const String &path, float size)
{
    ImGuiIO &io = ImGui::GetIO();

    mFonts[fontname] = io.Fonts->AddFontFromFileTTF(path.c_str(), size);
}

void Fonts::text(const String &txt, const String &fontname)
{
    ImGui::PushFont(mFonts[fontname]);
    ImGui::Text("%s", txt.c_str());
    ImGui::PopFont();
}

void Fonts::push(const String &fontname)
{
    ImGui::PushFont(mFonts[fontname]);
}

void Fonts::pop(void)
{
    ImGui::PopFont();
}

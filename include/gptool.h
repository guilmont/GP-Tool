#pragma once

#include "config.h"
#include "renderer/renderer.h"

#include <unordered_map>

class GPTool : public Renderer
{
public:
    GPTool(void);

    void onUserUpdate(float deltaTime) override;
    void ImGuiLayer(void) override;
    void ImGuiMenuLayer(void) override;

    void reset(const String &shader_path);

private:
    std::unique_ptr<Quad> quad = nullptr;
    std::unique_ptr<Shader> shader = nullptr;

    std::unordered_map<String, std::unique_ptr<Framebuffer>> fBuffer;
};
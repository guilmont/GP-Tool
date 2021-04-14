#pragma once

#include <glm/glm.hpp>

class Framebuffer
{
public:
    Framebuffer(uint32_t width, uint32_t height);
    ~Framebuffer(void);

    void bind(void);
    void unbind(void);

    inline uint32_t getID(void) { return textureID; }

    void setPosition(float x, float y) { position = {x, y}; }
    const glm::vec2 &getPosition(void) const { return position; }

    inline glm::vec2 getSize(void) const { return size; }

private:
    uint32_t bufferID = 0, textureID = 0;
    glm::vec2 size = {1.0f, 1.0f}, position = {0.0f, 0.0f};
};
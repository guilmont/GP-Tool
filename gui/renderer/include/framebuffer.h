#pragma once

#include <cstdint>

class Framebuffer
{
public:
    Framebuffer(uint32_t width, uint32_t height);
    ~Framebuffer(void);

    void bind(void);
    void unbind(void);

    inline uint32_t getWidth(void) { return width; }
    inline uint32_t getHeight(void) { return height; }
    inline uint32_t getID(void) { return textureID; }

private:
    uint32_t width, height;
    uint32_t bufferID = 0, textureID = 0;
};
#pragma once

#include <cstdint>
#include <string>
#include <vector>

using String = std::string;

struct TexInfo
{
    String path;
    uint32_t width, height, id;
};

class Texture
{
public:
    Texture(void) = default;
    ~Texture(void) = default;

    void cleanUp(void);
    uint32_t loadTexture(String filename);
    uint32_t createTexture(uint32_t width, uint32_t height);
    void updateTexture(uint32_t id, uint32_t *data);

    uint32_t createDepthComponent(uint32_t width, uint32_t height);
    void updateDepthComponent(uint32_t id, float *data);

    void bind(uint32_t id, uint32_t slot);

    inline String getPath(uint32_t id) { return vTexture.at(id).path; }
    inline uint32_t getWidth(uint32_t id) { return vTexture.at(id).width; }
    inline uint32_t getHeight(uint32_t id) { return vTexture.at(id).height; }
    inline uint32_t getBufferID(uint32_t id) { return vTexture.at(id).id; }

private:
    std::vector<TexInfo> vTexture;
};
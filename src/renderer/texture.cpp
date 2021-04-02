#include "texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "gl_assert.cpp"

void Texture::cleanUp(void)
{
    for (TexInfo tex : vTexture)
    {
        gl_call(glad_glDeleteTextures(1, &tex.id));
    }
} // destructor

void Texture::bind(uint32_t id, uint32_t slot)
{
    if (slot >= GL_MAX_TEXTURE_UNITS)
    {
        std::cerr << "ERROR: Texture unit overload!\n";
        exit(-1);
    }

    gl_call(glad_glActiveTexture(GL_TEXTURE0 + slot));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, vTexture.at(id).id));

} // bind

uint32_t Texture::loadTexture(String filename)
{
    int width, height, nComponents;
    stbi_set_flip_vertically_on_load(true);
    stbi_uc *data = stbi_load(filename.c_str(), &width, &height, &nComponents, 4);

    if (!data)
    {
        std::cerr << "Failed to load image " + filename << std::endl;
        exit(-1);
    }

    TexInfo tex;
    tex.path = filename;
    tex.width = width;
    tex.height = height;

    gl_call(glad_glGenTextures(1, &tex.id));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));

    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    gl_call(glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0,
                              GL_RGBA, GL_UNSIGNED_BYTE, data));

    gl_call(glad_glGenerateMipmap(GL_TEXTURE_2D));

    vTexture.push_back(tex);

    stbi_image_free(data);

    return vTexture.size() - 1;

} // constructor

uint32_t Texture::createTexture(uint32_t width, uint32_t height)
{
    TexInfo tex;
    tex.path = "";
    tex.width = width;
    tex.height = height;

    gl_call(glad_glGenTextures(1, &tex.id));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));

    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // I'll pass a black transparent texture
    float *mat = new float[width * height]();
    memset(mat, 0, width * height * sizeof(uint32_t));

    gl_call(glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                              tex.width, tex.height, 0,
                              GL_RGBA, GL_UNSIGNED_INT, mat));

    delete[] mat;

    vTexture.push_back(tex);
    return vTexture.size() - 1;

} // constructor

void Texture::updateTexture(uint32_t id, uint32_t *data)
{
    TexInfo &tex = vTexture.at(id);

    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));
    gl_call(glad_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.width, tex.height,
                                 GL_DEPTH_COMPONENT, GL_FLOAT, data));

} // updateData

uint32_t Texture::createDepthComponent(uint32_t width, uint32_t height)
{
    TexInfo tex;
    tex.path = "";
    tex.width = width;
    tex.height = height;

    gl_call(glad_glGenTextures(1, &tex.id));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));

    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // I'll pass a black texture
    float *mat = new float[width * height]();
    memset(mat, 0, width * height * sizeof(float));

    gl_call(glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                              tex.width, tex.height, 0,
                              GL_DEPTH_COMPONENT, GL_FLOAT, mat));

    delete[] mat;

    vTexture.push_back(tex);
    return vTexture.size() - 1;

} // constructor

void Texture::updateDepthComponent(uint32_t id, float *data)
{
    TexInfo &tex = vTexture.at(id);

    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));
    gl_call(glad_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.width, tex.height,
                                 GL_DEPTH_COMPONENT, GL_FLOAT, data));

} // updateData

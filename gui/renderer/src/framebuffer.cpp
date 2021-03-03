#include "framebuffer.h"

#include <glad/glad.h>
#include "gl_assert.cpp"

Framebuffer::Framebuffer(uint32_t width, uint32_t height) : width(width), height(height)
{
    gl_call(glad_glGenFramebuffers(1, &bufferID));
    gl_call(glad_glBindFramebuffer(GL_FRAMEBUFFER, bufferID));

    gl_call(glad_glGenTextures(1, &textureID));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, textureID));
    gl_call(glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                              GL_RGBA, GL_UNSIGNED_BYTE, NULL));

    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // Assigning texture to framebuffer
    gl_call(glad_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                        GL_TEXTURE_2D, textureID, 0));

    // Testing it worked properly
    if (glad_glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        std::cerr << "ERROR: Framebuffer is incomplete!!\n";
        exit(-1);
    }

    // binding standard buffer back
    gl_call(glad_glBindFramebuffer(GL_FRAMEBUFFER, 0));

} // constructor

Framebuffer::~Framebuffer(void)
{
    gl_call(glad_glDeleteFramebuffers(1, &bufferID));
    gl_call(glad_glDeleteTextures(1, &textureID));
} // destructor

void Framebuffer::bind(void)
{
    gl_call(glad_glBindFramebuffer(GL_FRAMEBUFFER, bufferID));
    gl_call(glad_glViewport(0, 0, width, height));
}

void Framebuffer::unbind(void)
{
    gl_call(glad_glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

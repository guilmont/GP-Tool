#pragma once

#include <iostream>
#include <glad/glad.h>

///////////////////////////////////////////////////////////////////////////////
// OPENGL ERROR HANDLING -> MacOSX doesn't handle the newer approach

#define gl_call(x)  \
    GlClearError(); \
    x;              \
    GlLogCall(#x, __FILE__, __LINE__)

static void GlClearError()
{
    while (glad_glGetError() != GL_NO_ERROR)
    {
    }
}

static void GlLogCall(const char *func, const char *file, int line)
{
    while (GLenum error = glad_glGetError())
        if (error != GL_NO_ERROR)
        {
            printf("OpenGL ERROR %d: %s -> line %d :: %s\n", error, func, line, file);
            exit(-1);
        }
}
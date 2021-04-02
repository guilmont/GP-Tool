#include "shader.h"
#include "gl_assert.cpp"

#include <fstream>
#include <sstream>
#include <cstring>

static void CheckShaderError(uint32_t shader, uint32_t flag, bool isProgram, String msg)
{
    int success = 0;
    char error[1024] = {0};

    if (isProgram)
    {
        gl_call(glad_glGetProgramiv(shader, flag, &success));
    }
    else
    {
        gl_call(glad_glGetShaderiv(shader, flag, &success));
    }

    if (success == GL_FALSE)
    {
        if (isProgram)
        {
            gl_call(glad_glGetProgramInfoLog(shader, sizeof(error), NULL, error));
        }
        else
        {
            gl_call(glad_glGetShaderInfoLog(shader, sizeof(error), NULL, error));
        }

        std::cerr << msg << error;
        exit(0);
    } // if-not-success

} // checkShaderError

static uint32_t CreateShader(const char *shaderSource, GLenum shaderType)
{
    gl_call(uint32_t shader = glad_glCreateShader(shaderType));
    if (shader == 0)
    {
        std::cout << "Failed to create shader!\n";
        exit(0);
    }

    int sourceLength = strlen(shaderSource);
    gl_call(glShaderSource(shader, 1, &shaderSource, &sourceLength));

    String error = "\nError: Shader compilation failed => ";

    gl_call(glCompileShader(shader));
    CheckShaderError(shader, GL_COMPILE_STATUS, false, error);

    return shader;
} // CreateShader

static uint32_t genShader(const String &vertex_shader, const String &frag_shader)
{

    auto readShader = [](const String &path) -> String {
        std::ifstream arq(path);
        std::stringstream shader;
        shader << arq.rdbuf();
        arq.close();
        return shader.str();
    };

    uint32_t vtx = CreateShader(readShader(vertex_shader).c_str(), GL_VERTEX_SHADER);
    uint32_t frg = CreateShader(readShader(frag_shader).c_str(), GL_FRAGMENT_SHADER);

    // Create program
    gl_call(uint32_t program = glad_glCreateProgram());
    gl_call(glad_glAttachShader(program, vtx));
    gl_call(glad_glAttachShader(program, frg));

    // Link shaders to program
    String error = "ERROR: Cannot link shader programs => ";

    gl_call(glad_glLinkProgram(program));
    CheckShaderError(program, GL_LINK_STATUS, true, error);

    // // Validating program
    // gl_call(glad_glValidateProgram(program));
    // CheckShaderError(program, GL_VALIDATE_STATUS, true,
    //                  "Error: Program is invalid: ");

    gl_call(glad_glDeleteShader(vtx));
    gl_call(glad_glDeleteShader(frg));

    return program;

} // genRasterShader

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Shader::Shader(void)
{
    String path(PROJECT_DIR);
    vProgram["basic"] = genShader(path + "/assets/shaders/basic.vtx.glsl",
                                  path + "/assets/shaders/basic.frag.glsl");
    // vProgram["viewport"] = genShader(basic_vert, viewport_frag);
    // vProgram["histogram"] = genShader(basic_vert, histogram_frag);
    // vProgram["trajectory"] = genShader(basic_vert, trajectory_frag);
    // vProgram["selectRoi"] = genShader(basic_vert, selectRoi_frag);

} // constructor

Shader::~Shader(void)
{
    for (auto &it : vProgram)
        glad_glDeleteProgram(it.second);

} // destructor

void Shader::useProgram(const String &name)
{
    program_used = vProgram[name];
    gl_call(glUseProgram(program_used));
} // useProgram

///////////////////////////////////////////////////////////////////////////////
// UNIFORMS

void Shader::setInteger(const String &name, int val)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform1i(loc, val));

} // setUniform1i

void Shader::setFloat(const String &name, float val)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform1f(loc, val));

} // setUniform1i

void Shader::setVec2f(const String &name, const float *v)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform2f(loc, v[0], v[1]));
} // setUnifom3f

void Shader::setVec3f(const String &name, const float *v)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform3f(loc, v[0], v[1], v[2]));
} // setUnifom3f

void Shader::setVec4f(const String &name, const float *v)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform4f(loc, v[0], v[1], v[2], v[3]));
} // setUnifom3f

void Shader::setMatrix3f(const String &name, const float *mat)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniformMatrix3fv(loc, 1, GL_FALSE, mat));
} //setUniformMatrix4f

void Shader::setMatrix4f(const String &name, const float *mat)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniformMatrix4fv(loc, 1, GL_FALSE, mat));
} //setUniformMatrix4f

void Shader::setFloatArray(const String &name, const float *ptr, size_t N)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform1fv(loc, N, ptr));
}

void Shader::setVec2fArray(const String &name, const float *ptr, size_t N)
{
    gl_call(uint32_t loc = glad_glGetUniformLocation(program_used, name.c_str()));
    gl_call(glad_glUniform2fv(loc, N, ptr));
}
//
// Created by andreikingsley on 15.11.2021.
//

#ifndef PRACTICE9_SHADER_H
#define PRACTICE9_SHADER_H

#endif //PRACTICE9_SHADER_H

#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/mat4x4.hpp>

class ShaderProgram
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    ShaderProgram(const char* vertexCode, const char* fragmentCode)
    {
        auto vertex = create_shader(GL_VERTEX_SHADER, vertexCode);
        auto fragment = create_shader(GL_FRAGMENT_SHADER, fragmentCode);
        auto program = create_program(vertex, fragment);
        ID = program;
    }

    static GLuint create_shader(GLenum type, const char *source) {
        GLuint result = glCreateShader(type);
        glShaderSource(result, 1, &source, nullptr);
        glCompileShader(result);
        GLint status;
        glGetShaderiv(result, GL_COMPILE_STATUS, &status);
        if (status != GL_TRUE) {
            GLint info_log_length;
            glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
            std::string info_log(info_log_length, '\0');
            glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
            throw std::runtime_error("ShaderProgram compilation failed: " + info_log);
        }
        return result;
    }

    static GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
        GLuint result = glCreateProgram();
        glAttachShader(result, vertex_shader);
        glAttachShader(result, fragment_shader);
        glLinkProgram(result);

        GLint status;
        glGetProgramiv(result, GL_LINK_STATUS, &status);
        if (status != GL_TRUE) {
            GLint info_log_length;
            glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
            std::string info_log(info_log_length, '\0');
            glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
            throw std::runtime_error("Program linkage failed: " + info_log);
        }

        return result;
    }
    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    {
        glUseProgram(ID);
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setMatrix(const std::string &name,
                   glm::mat4 &mat4) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, reinterpret_cast<float *>(&mat4));
    }

    void setVec3(const std::string &name,
                 glm::vec3 &vec3) const {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), vec3.x, vec3.y, vec3.z);
    }

private:

};
#endif


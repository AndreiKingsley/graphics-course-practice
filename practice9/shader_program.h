//
// Created by andreikingsley on 16.11.2021.
//

#ifndef PRACTICE9_SHADER_PROGRAM_H
#define PRACTICE9_SHADER_PROGRAM_H

#include <GL/glew.h>
#include <unordered_map>
#include <vector>
#include <glm/detail/type_mat4x4.hpp>

struct shader_program {
    GLuint id;
    std::unordered_map<std::string, GLuint> name_to_location;

    GLuint create_shader(GLenum type, const char *source) {
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
            throw std::runtime_error("Shader compilation failed: " + info_log);
        }
        return result;
    }

    GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
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

    void create(const char *vertex_shader_source, const char *fragment_shader_source) {
        auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
        auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
        id = create_program(vertex_shader, fragment_shader);
    }

    void use() {
        glUseProgram(id);
    }

    /*
    void setup_locations(const std::vector<const char *> &uniforms_names) {
        for (const GLchar *const uniform_name: uniforms_names) {
            name_to_location[uniform_name] = glGetUniformLocation(id, uniform_name);
        }
    }
    */

    GLuint location(const char *name) {
        if (name_to_location.contains(name)){
            return name_to_location[name];
        }
        auto location = glGetUniformLocation(id, name);
        name_to_location[name] = location;
        return location;
    }

    void set_int(const char *name, int value) {
        glUniform1i(location(name), value);
    }

    void set_float(const char *name, float value) {
        glUniform1f(location(name), value);
    }

    void set_matrix(const char *name, glm::mat4 &mat4) {
        glUniformMatrix4fv(location(name), 1, GL_FALSE, reinterpret_cast<float *>(&mat4));
    }

    void set_vec3(const char *name, glm::vec3 &vec3) {
        glUniform3f(location(name), vec3.x, vec3.y, vec3.z);
    }

    /*
    void setInt(const std::string &name, int value) const
    {
        glUniform1i(glGetUniformLocation(id, name.c_str()), value);
    }

    void setMatrix(const std::string &name,
                   glm::mat4 &mat4) const {
        glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, reinterpret_cast<float *>(&mat4));
    }

    void setVec3(const std::string &name,
                 glm::vec3 &vec3) const {
        glUniform3f(glGetUniformLocation(id, name.c_str()), vec3.x, vec3.y, vec3.z);
    }
    */
};

#endif //PRACTICE9_SHADER_PROGRAM_H

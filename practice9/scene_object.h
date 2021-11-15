//
// Created by andreikingsley on 15.11.2021.
//


#ifndef PRACTICE9_SCENE_OBJECT_H
#define PRACTICE9_SCENE_OBJECT_H


#include <GL/glew.h>

#include "mesh.h"

struct scene_object {
    GLuint vao, vbo, ebo;
    std::vector<texture_desc> textures{};
    GLuint indices_size;

    int has_normal_tex = 0;

    void init(const Mesh &mesh) {
        std::vector<vertex> vertices;
        std::vector<unsigned int> indices = mesh.indices;

        for (auto t: mesh.textures) {
            textures.push_back({t.id, t.type});
            if (t.type == "texture_normal") {
                has_normal_tex = 1;
            }
        }

        for (auto v: mesh.vertices) {
            vertices.push_back({v.Position, v.Normal, v.TexCoords});
        }

        indices_size = indices.size();

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *) offsetof(vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *) offsetof(vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)offsetof(vertex, texcoords));
    }

    void draw(GLuint program){

        for(unsigned int i = 0; i < textures.size(); i++){
            glActiveTexture(GL_TEXTURE0 + i);
            glUniform1i(glGetUniformLocation(program, textures[i].name.c_str()), i);
            glBindTexture(GL_TEXTURE_2D, textures[i].id);
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indices_size, GL_UNSIGNED_INT, 0);
        // TODO
        glBindVertexArray(0);
        glActiveTexture(GL_TEXTURE0);
    }

};


#endif //PRACTICE9_SCENE_OBJECT_H

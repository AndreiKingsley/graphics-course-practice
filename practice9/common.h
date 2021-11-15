//
// Created by andreikingsley on 15.11.2021.
//

#ifndef PRACTICE9_COMMON_H
#define PRACTICE9_COMMON_H
struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords;
};

struct texture_desc {
    GLuint id;
    std::string name;
};

#endif //PRACTICE9_COMMON_H

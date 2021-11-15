//
// Created by andreikingsley on 15.11.2021.
//

#ifndef PRACTICE9_SCENE_H
#define PRACTICE9_SCENE_H

#include "scene_object.h"
#include "model.h"

struct scene {
    std::vector<scene_object> objects {};

    void load(const std::string& dir_path, const std::string& file_name) {
        Model model(dir_path + "/" + file_name);
        for (const auto& mesh:model.meshes) {
            scene_object new_object;
            new_object.init(mesh);
            objects.push_back(new_object);
        }
    }
};

#endif //PRACTICE9_SCENE_H

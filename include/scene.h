#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "objects.h"

struct Scene {
public:
    std::vector<Shape *> shapes;
    Scene();
    Scene(std::vector<Shape *> shapes);
    void LoadModel(std::string path);
    void LoadTestModel();
};

#endif
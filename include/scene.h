#ifndef SCENE_H
#define SCENE_H

#include <vector>

#include "bvh.h"
#include "objects.h"

struct Scene {
 public:
  std::vector<Object *> objects;
  Scene();
  Scene(std::vector<Object *> objects);
  bool intersect(Ray ray, Intersection &intersection);
  void createBVH();
  void LoadModel(std::string path);
  void LoadTest();

 private:
  BVH *bvh = NULL;
};

#endif
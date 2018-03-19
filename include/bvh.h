#ifndef BVH_H
#define BVH_H

#include <glm/glm.hpp>
#include <vector>
#include "objects.h"
#include "ray.h"

struct Extents {
  Extents();
  bool intersect(Ray *ray, float *precomputedNumerator, float *precomputeDenominator,
                   float &tNear, float &tFar);
  float d[7][2];
};

struct BVH {
 private:
  Extents *extents;
  uint32_t sceneSize;

 public:
  BVH(std::vector<Object *> scene);
  bool intersect(Ray *ray, Intersection &intersection, std::vector<Object *> scene);
};

#endif
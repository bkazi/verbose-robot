#ifndef BVH_H
#define BVH_H

#include <glm/glm.hpp>
#include <vector>
#include "objects.h"

struct Extents {
  Extents();
  bool intersect(Ray ray, float *precomputedNumerator,
                 float *precomputeDenominator, float &tNear, float &tFar,
                 uint8_t &planeIndex);
  float d[7][2];
};

struct BVH {
 private:
  Extents *extents;

 public:
  BVH(std::vector<Object *> scene);
  Object *intersect(Ray ray, std::vector<Object *> scene);
};

#endif
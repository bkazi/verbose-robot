#ifndef BVH_H
#define BVH_H

#include <glm/glm.hpp>
#include <vector>
#include "objects.h"

struct Extents {
  Extents();
  bool intersect(const glm::vec4 start, const glm::vec4 direction,float *precomputedNumerator, float *precomputeDenominator,
                   float &tNear, float &tFar);
  float d[7][2];
};

struct BVH {
 private:
  Extents *extents;
  uint32_t sceneSize;

 public:
  BVH(std::vector<Object *> scene);
  std::vector<uint32_t> intersect(const glm::vec4 start, const glm::vec4 direction);
};

#endif
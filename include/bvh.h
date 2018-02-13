#ifndef BVH_H
#define BVH_H

#include <glm/glm.hpp>
#include <vector>
#include "objects.h"

struct Extents {
  Extents();
  bool intersect(const glm::vec4 start, const glm::vec4 direction, float &tNear,
                 float &tFar, uint8_t &planeIndex);
  float d[7][2];
};

struct BVH {
 private:
  Extents *extents;

 public:
  BVH(std::vector<Shape *> shapes);
  const bool intersect(const glm::vec4 start, const glm::vec4 direction);
};

#endif
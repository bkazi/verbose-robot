#ifndef RAY_H
#define RAY_H

#include <glm/glm.hpp>
#include "objects.h"

struct Ray {
public:
    glm::vec4 position;
    glm::vec4 direction;
    Ray(glm::vec4 position, glm::vec4 direction);
};

struct Intersection {
  glm::vec4 position;
  float distance;
  Primitive *primitive;
};

#endif
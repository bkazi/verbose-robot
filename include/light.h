#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

#define LIGHTMAP_SIZE 1024

struct Light {
  glm::vec4 position;
  glm::vec4 direction;
  glm::vec3 power;
  float depthBuffer[LIGHTMAP_SIZE * LIGHTMAP_SIZE];
  bool needsUpdate;

  void transformationMatrix(glm::mat4 &transMat);
  int test(glm::vec4 vertexWorldPos);
};

#endif
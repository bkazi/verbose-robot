#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>

#define LIGHTMAP_SIZE 256

struct Light {
  glm::vec4 position;
  glm::vec3 power;
  float *depthBuffer[6];
  bool needsUpdate;
  glm::vec3 rot[6] = {glm::vec3(0, -M_PI/2, 0), glm::vec3(0, M_PI/2, 0), glm::vec3(-M_PI/2, 0, 0), glm::vec3(M_PI/2, 0, 0), glm::vec3(0, 0, 0), glm::vec3(0, M_PI, 0)};

  Light();
  int getCubeIndex(glm::vec4 pos);
  void transformationMatrix(glm::mat4 &transMat, glm::vec3 r);
  int test(glm::vec4 vertexWorldPos);
};

#endif
#ifndef UTIL_H
#define UTIL_H

#include <glm/glm.hpp>

struct Pixel {
  int x;
  int y;
  float zinv;
  glm::vec4 homogenous;
  glm::vec4 worldPos;
  glm::vec4 normal;
  glm::vec3 reflectance;

  Pixel();
  Pixel(glm::ivec2 vec);
  Pixel(glm::ivec2 vec, float zinv, glm::vec4 homogenous, glm::vec4 worldPos, glm::vec4 normal, glm::vec3 reflectance);
};

struct Vertex {
  glm::vec4 position;
  glm::vec4 normal;
  glm::vec3 reflectance;

  Vertex();
  Vertex(glm::vec4 position);
  Vertex(glm::vec4 position, glm::vec4 normal, glm::vec3 reflectance);
};

glm::mat4 CalcRotationMatrix(glm::vec3 rotation);
void TransformationMatrix(glm::vec3 rotation, glm::vec4 position, glm::mat4 &M);

#endif
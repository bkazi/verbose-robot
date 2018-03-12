#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

struct Camera {
 public:
  Camera(glm::vec4 position, glm::vec3 rotation, float focalLength,
         float movementSpeed, float rotationSpeed);

  void update(float dt);
  glm::mat4 getTransformationMatrix();
  glm::mat4 getTranslationMatrix();
  glm::mat4 getRotationMatrix();
  float focalLength;
  glm::vec4 position;
  glm::vec3 rotation;

 private:
  float movementSpeed;
  float rotationSpeed;
};

#endif
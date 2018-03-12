#include <SDL.h>

#include "camera.h"

using glm::mat4;
using glm::vec3;
using glm::vec4;

Camera::Camera(vec4 position, vec3 rotation, float focalLength,
               float movementSpeed, float rotationSpeed)
    : position(position),
      rotation(rotation),
      focalLength(focalLength),
      movementSpeed(movementSpeed),
      rotationSpeed(rotationSpeed) {}

void Camera::update(float dt) {
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_W]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      rotation.x -= rotationSpeed * dt;
    } else {
      position.z += movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_S]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      rotation.x += rotationSpeed * dt;
    } else {
      position.z -= movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_A]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      rotation.y += rotationSpeed * dt;
    } else {
      position.x -= movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_D]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      rotation.y -= rotationSpeed * dt;
    } else {
      position.x += movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_Q]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      rotation.z += rotationSpeed * dt;
    } else {
      position.y += movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_E]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      rotation.z -= rotationSpeed * dt;
    } else {
      position.y -= movementSpeed * dt;
    }
  }
}

mat4 Camera::getRotationMatrix() {
  float x = rotation.x;
  float y = rotation.y;
  float z = rotation.z;
  mat4 Rx = mat4(vec4(1, 0, 0, 0), vec4(0, cosf(x), sinf(x), 0), vec4(0, -sinf(x), cosf(x), 0), vec4(0, 0, 0, 1));
  mat4 Ry = mat4(vec4(cosf(y), 0, -sinf(y), 0), vec4(0, 1, 0, 0), vec4(sinf(y), 0, cosf(y), 0), vec4(0 , 0, 0, 1));
  mat4 Rz = mat4(vec4(cosf(z), sinf(z), 0, 0), vec4(-sinf(z), cosf(z), 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
  return Rz * Ry * Rx;
}

mat4 Camera::getTranslationMatrix() {
    mat4 trans = mat4(1);
    trans[3] = -1.f * position;
    return trans;
}

mat4 Camera::getTransformationMatrix() {
  return getRotationMatrix() + getTranslationMatrix();
}
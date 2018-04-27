#include "light.h"

#include <assert.h>
#include <iostream>
#include <SDL.h>

#include "util.h"

using namespace std;
using glm::ivec2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

void Light::transformationMatrix(mat4 &transMat) {
  mat4 rot = CalcRotationMatrix(vec3(direction));
  rot[3] = -1.f * position;
  transMat = rot;
}

int Light::test(vec4 vertexWorldPos) {
  mat4 transMat;
  transformationMatrix(transMat);
  vec4 v = transMat * vertexWorldPos;
  Pixel p;
  p.zinv = 1.f / v.z;
  p.x = int(LIGHTMAP_SIZE * v.x * p.zinv) + (LIGHTMAP_SIZE / 2);
  p.y = int(LIGHTMAP_SIZE * v.y * p.zinv) + (LIGHTMAP_SIZE / 2);
  if (p.x >= 0 && p.x < LIGHTMAP_SIZE && p.y >= 0 && p.y < LIGHTMAP_SIZE) {
    return p.zinv + 2e-4f >=
           depthBuffer[p.y * LIGHTMAP_SIZE + p.x];
  }
  return true;
}

bool Light::update(float dt) {
  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  bool updated = false;
  if (keystate[SDL_SCANCODE_UP]) {
    position.z += movementSpeed * dt;
    updated = true;
  }

  if (keystate[SDL_SCANCODE_DOWN]) {
    position.z -= movementSpeed * dt;
    updated = true;
  }

  if (keystate[SDL_SCANCODE_LEFT]) {
    position.x -= movementSpeed * dt;

    updated = true;
  }

  if (keystate[SDL_SCANCODE_RIGHT]) {
    position.x += movementSpeed * dt;
    updated = true;
  }

  if (keystate[SDL_SCANCODE_PAGEUP]) {
    position.y += movementSpeed * dt;
    updated = true;
  }

  if (keystate[SDL_SCANCODE_PAGEDOWN]) {
    position.y -= movementSpeed * dt;
    updated = true;
  }

  needsUpdate = updated;
  return updated;
}
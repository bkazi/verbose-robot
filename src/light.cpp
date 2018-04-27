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

#define LIGHTMAP_SIZE 256

Light::Light() {
  for (int i = 0; i < 6; i++) {
    depthBuffer[i] = new float[LIGHTMAP_SIZE * LIGHTMAP_SIZE];
  }
}

int Light::getCubeIndex(vec4 pos) {
  float absX = abs(pos.x);
  float absY = abs(pos.y);
  float absZ = abs(pos.z);

  int isXPositive = pos.x > 0 ? 1 : 0;
  int isYPositive = pos.y > 0 ? 1 : 0;
  int isZPositive = pos.z > 0 ? 1 : 0;

  // POSITIVE X
  if (isXPositive && absX >= absY && absX >= absZ) {
    // u (0 to 1) goes from +z to -z
    // v (0 to 1) goes from -y to +y
    return 1;
  }
  // NEGATIVE X
  if (!isXPositive && absX >= absY && absX >= absZ) {
    // u (0 to 1) goes from -z to +z
    // v (0 to 1) goes from -y to +y
    return 0;
  }
  // POSITIVE Y
  if (isYPositive && absY >= absX && absY >= absZ) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from +z to -z
    return 2;
  }
  // NEGATIVE Y
  if (!isYPositive && absY >= absX && absY >= absZ) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from -z to +z
    return 3;
  }
  // POSITIVE Z
  if (isZPositive && absZ >= absX && absZ >= absY) {
    // u (0 to 1) goes from -x to +x
    // v (0 to 1) goes from -y to +y
    return 4;
  }
  // NEGATIVE Z
  if (!isZPositive && absZ >= absX && absZ >= absY) {
    // u (0 to 1) goes from +x to -x
    // v (0 to 1) goes from -y to +y
    return 5;
  }
  return 0;
}

void Light::transformationMatrix(mat4 &transMat, vec3 r) {
  mat4 rot = CalcRotationMatrix(r);
  rot[3] = -1.f * position;
  transMat = rot;
}

int Light::test(vec4 vertexWorldPos) {
  vec4 v = vertexWorldPos - position;
  Pixel p;
  p.zinv = 1.f / v.z;
  p.x = int(LIGHTMAP_SIZE * v.x * p.zinv) + (LIGHTMAP_SIZE / 2);
  p.y = int(LIGHTMAP_SIZE * v.y * p.zinv) + (LIGHTMAP_SIZE / 2);
  if (p.x >= 0 && p.x < LIGHTMAP_SIZE && p.y >= 0 && p.y < LIGHTMAP_SIZE) {
    return p.zinv - 0.5f >=
           depthBuffer[getCubeIndex(v)][p.y * LIGHTMAP_SIZE + p.x];
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
#include <iostream>
#include <glm/glm.hpp>
#include <stdint.h>
#include <assert.h>

#include "objects.h"

using namespace std;
using glm::ivec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

#define LIGHTMAP_SIZE 256

struct Light {
  vec4 position;
  vec3 power;
  float *depthBuffer[6];
  bool needsUpdate;
  vec3 rot[6] = {vec3(0, -M_PI/2, 0), vec3(0, M_PI/2, 0), vec3(-M_PI/2, 0, 0), vec3(M_PI/2, 0, 0), vec3(0, 0, 0), vec3(0, M_PI, 0)};

  Light() {
    for (int i = 0; i < 6; i++) {
      depthBuffer[i] = new float[LIGHTMAP_SIZE * LIGHTMAP_SIZE];
    }
  }

  int getCubeIndex(vec3 pos) {
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
  };

  void transformationMatrix(mat4 &transMat, vec3 r) {
    mat4 rot = CalcRotationMatrix(r);
    rot[3] = -1.f * position;
    transMat = rot;
  }

  int test(vec4 vertexWorldPos) {
    vec4 v = vertexWorldPos - position;
    Pixel p;
    p.zinv = 1.f / v.z;
    p.x = int(LIGHTMAP_SIZE * v.x * p.zinv) + (LIGHTMAP_SIZE / 2);
    p.y = int(LIGHTMAP_SIZE * v.y * p.zinv) + (LIGHTMAP_SIZE / 2);
    if (p.x >= 0 && p.x < LIGHTMAP_SIZE && p.y >= 0 && p.y < LIGHTMAP_SIZE) {
      return p.zinv - 0.5f >= depthBuffer[getCubeIndex(v)][p.y * LIGHTMAP_SIZE + p.x];
    }
    return true;
  };
};

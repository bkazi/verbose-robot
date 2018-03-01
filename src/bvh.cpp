#include "bvh.h"

using namespace std;
using glm::vec3;

const float rt3b3 = sqrtf(3) / 3.f;

const uint normalSize = 7;
const vec3 planeSetNormals[7] = {vec3(1, 0, 0),
                                 vec3(0, 1, 0),
                                 vec3(0, 0, 1),
                                 vec3(rt3b3, rt3b3, rt3b3),
                                 vec3(-rt3b3, rt3b3, rt3b3),
                                 vec3(-rt3b3, -rt3b3, rt3b3),
                                 vec3(rt3b3, -rt3b3, rt3b3)};

Extents::Extents() {
  for (uint8_t i = 0; i < normalSize; i++)
    d[i][0] = INFINITY, d[i][1] = -INFINITY;
}

bool Extents::intersect(const glm::vec4 start, const glm::vec4 direction,
                   float *precomputedNumerator, float *precomputeDenominator,
                   float &tNear, float &tFar) {
  for (uint8_t i = 0; i < normalSize; ++i) {
    float tn = (d[i][0] - precomputedNumerator[i]) / precomputeDenominator[i];
    float tf = (d[i][1] - precomputedNumerator[i]) / precomputeDenominator[i];
    if (precomputeDenominator[i] < 0) std::swap(tn, tf);
    if (tn > tNear) tNear = tn;
    if (tf < tFar) tFar = tf;
    if (tNear > tFar) return false;
  }

  return true;
}

BVH::BVH(vector<Object *> scene) {
  extents = new Extents[scene.size()];
  for (uint32_t i = 0; i < scene.size(); ++i) {
    for (uint8_t j = 0; j < normalSize; ++j) {
      scene[i]->computeBounds(planeSetNormals[j], extents[i].d[j][0],
                              extents[i].d[j][1]);
    }
  }
}

Object *BVH::intersect(vector<Object *> scene, const glm::vec4 start,
                       const glm::vec4 direction) {
  float tClosest = INFINITY;
  Object *hitObject = NULL;
  float precomputedNumerator[normalSize], precomputeDenominator[normalSize];

  for (uint8_t i = 0; i < normalSize; ++i) {
    precomputedNumerator[i] =
        dot(planeSetNormals[i], vec3(start.x, start.y, start.z));
    precomputeDenominator[i] =
        dot(planeSetNormals[i], vec3(direction.x, direction.y, direction.z));
  }

  for (uint32_t i = 0; i < scene.size(); ++i) {
    // __sync_fetch_and_add(&numRayVolumeTests, 1); -- I no understand
    float tNear = -INFINITY, tFar = INFINITY;
    if (extents[i].intersect(start, direction, precomputedNumerator,
                             precomputeDenominator, tNear, tFar)) {
      if (tNear < tClosest) tClosest = tNear;
      hitObject = scene[i];
    }
  }

  return hitObject;
}
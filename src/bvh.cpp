#include "bvh.h"
#include <iostream>

using namespace std;
using glm::vec3;

const float rt3b3 = sqrtf(3) / 3.f;

const uint32_t normalSize = 7;
const vec3 planeSetNormals[normalSize] = {vec3(1, 0, 0),
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

bool Extents::intersect(Ray ray, float *precomputedNumerator,
                        float *precomputeDenominator, float &tNear, float &tFar,
                        uint8_t &planeIndex) {
  for (uint8_t i = 0; i < normalSize; ++i) {
    float tn = (d[i][0] - precomputedNumerator[i]) / precomputeDenominator[i];
    float tf = (d[i][1] - precomputedNumerator[i]) / precomputeDenominator[i];
    if (precomputeDenominator[i] < 0) std::swap(tn, tf);
    if (tn > tNear) tNear = tn, planeIndex = i;
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

Object *BVH::intersect(Ray ray, vector<Object *> scene) {
  float tClosest = INFINITY;
  Object *hitObject = NULL;
  float precomputedNumerator[normalSize], precomputeDenominator[normalSize];
  for (uint8_t i = 0; i < normalSize; ++i) {
    precomputedNumerator[i] = dot(planeSetNormals[i], vec3(ray.position));
    precomputeDenominator[i] = dot(planeSetNormals[i], vec3(ray.direction));
  }
  for (uint32_t i = 0; i < scene.size(); ++i) {
    float tNear = -INFINITY, tFar = INFINITY;
    uint8_t planeIndex;
    if (extents[i].intersect(ray, precomputedNumerator, precomputeDenominator,
                             tNear, tFar, planeIndex)) {
      if (tNear < tClosest) {
        tClosest = tNear;
        hitObject = scene[i];
      }
    }
  }

  return hitObject;
}

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

bool Extents::intersect(Ray *ray,
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

bool BVH::intersect(Ray *ray, Intersection &intersection, vector<Object *> scene) {
  float tClosest = INFINITY;
  float precomputedNumerator[normalSize], precomputeDenominator[normalSize];

  intersection.distance = 0;

  for (uint8_t i = 0; i < normalSize; ++i) {
    precomputedNumerator[i] =
        glm::dot(planeSetNormals[i], vec3(ray->position.x, ray->position.y, ray->position.z));
    precomputeDenominator[i] =
        glm::dot(planeSetNormals[i], vec3(ray->direction.x, ray->direction.y, ray->direction.z));
  }

  for (uint32_t i = 0; i < scene.size(); ++i) {
    // __sync_fetch_and_add(&numRayVolumeTests, 1); -- I no understand
    float tNear = -INFINITY, tFar = INFINITY;
    if (extents[i].intersect(ray, precomputedNumerator,
                             precomputeDenominator, tNear, tFar)) {
      if (tNear < tClosest) tClosest = tNear;
      // TODO get ray hit
    }
  }

  return intersection.distance != 0;
}
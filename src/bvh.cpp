#include "bvh.h"
#include <atomic>
#include <glm/gtx/string_cast.hpp>
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

/* BOUNDING BOX IMPLEMENTATION */

BBox::BBox() {}
BBox::BBox(vec3 min, vec3 max) {
  bounds[0] = min;
  bounds[1] = max;
}
BBox& BBox::extendBy(const vec3& point) {
  if (point.x < bounds[0].x) bounds[0].x = point.x;
  if (point.y < bounds[0].y) bounds[0].y = point.y;
  if (point.z < bounds[0].z) bounds[0].z = point.z;
  if (point.x > bounds[1].x) bounds[1].x = point.x;
  if (point.y > bounds[1].y) bounds[1].y = point.y;
  if (point.z > bounds[1].z) bounds[1].z = point.z;

  return *this;
}
vec3 BBox::centroid() const { return (bounds[0] + bounds[1]) * 0.5f; }

bool BBox::intersect(const vec3& start, const vec3& direction, const vec3& sign,
                     float& minDist) const {
  int idx;
  float tmin, tmax, tymin, tymax, tzmin, tzmax;

  idx = round(sign.x);
  tmin = (bounds[idx].x - start.x) * direction.x;

  idx = round(1 - sign.x);
  tmax = (bounds[idx].x - start.x) * direction.x;

  idx = round(sign.y);
  tymin = (bounds[idx].y - start.y) * direction.y;

  idx = round(1 - sign.y);
  tymax = (bounds[idx].y - start.y) * direction.y;

  if ((tmin > tymax) || (tymin > tmax)) return false;

  if (tymin > tmin) tmin = tymin;
  if (tymax < tmax) tmax = tymax;

  idx = round(sign.z);
  tzmin = (bounds[idx].z - start.z) * direction.z;

  idx = round(1 - sign.z);
  tzmax = (bounds[idx].z - start.z) * direction.z;

  if ((tmin > tzmax) || (tzmin > tmax)) return false;

  if (tzmin > tmin) tmin = tzmin;
  if (tzmax < tmax) tmax = tzmax;

  minDist = tmin;

  return true;
}

/* EXTENTS IMPLEMENTATION */

BVH::Extents::Extents() {
  for (uint8_t i = 0; i < normalsSize; ++i)
    slabs[i][0] = INFINITY, slabs[i][1] = -INFINITY;
}
void BVH::Extents::extendBy(const Extents& extents) {
  for (uint8_t i = 0; i < normalsSize; ++i) {
    if (extents.slabs[i][0] < slabs[i][0]) slabs[i][0] = extents.slabs[i][0];
    if (extents.slabs[i][1] > slabs[i][1]) slabs[i][1] = extents.slabs[i][1];
  }
}
vec3 BVH::Extents::centroid() const {
  return vec3(slabs[0][0] + slabs[0][1] * 0.5, slabs[1][0] + slabs[1][1] * 0.5,
              slabs[2][0] + slabs[2][1] * 0.5);
}

bool BVH::Extents::intersect(const float* precomputedNumerator,
                             const float* precomputedDenominator, float& tNear,
                             float& tFar, uint8_t& planeIndex) const {
  for (uint8_t i = 0; i < normalsSize; ++i) {
    float tNearExtents =
        (slabs[i][0] - precomputedNumerator[i]) / precomputedDenominator[i];
    float tFarExtents =
        (slabs[i][1] - precomputedNumerator[i]) / precomputedDenominator[i];
    if (precomputedDenominator[i] < 0) std::swap(tNearExtents, tFarExtents);
    if (tNearExtents > tNear) tNear = tNearExtents, planeIndex = i;
    if (tFarExtents < tFar) tFar = tFarExtents;
    if (tNear > tFar) return false;
  }

  return true;
}

/* OCTREE IMPLEMENTATION */

BVH::Octree::Octree(const Extents& sceneExtents) {
  float xDiff = sceneExtents.slabs[0][1] - sceneExtents.slabs[0][0];
  float yDiff = sceneExtents.slabs[1][1] - sceneExtents.slabs[1][0];
  float zDiff = sceneExtents.slabs[2][1] - sceneExtents.slabs[2][0];
  float maxDiff = std::max(xDiff, std::max(yDiff, zDiff));
  vec3 minPlusMax(sceneExtents.slabs[0][0] + sceneExtents.slabs[0][1],
                  sceneExtents.slabs[1][0] + sceneExtents.slabs[1][1],
                  sceneExtents.slabs[2][0] + sceneExtents.slabs[2][1]);
  bbox[0] = (minPlusMax - maxDiff) * 0.5f;
  bbox[1] = (minPlusMax + maxDiff) * 0.5f;
  root = new OctreeNode;
}

BVH::Octree::~Octree() { deleteOctreeNode(root); }

void BVH::Octree::insert(const Extents* extents) {
  insert(root, extents, bbox, 0);
}
void BVH::Octree::build() { build(root, bbox); }

void BVH::Octree::deleteOctreeNode(OctreeNode*& node) {
  for (uint8_t i = 0; i < 8; i++) {
    if (node->child[i] != nullptr) {
      deleteOctreeNode(node->child[i]);
    }
  }
  delete node;
}

BVH::Octree::QueueElement::QueueElement(const OctreeNode* node, float distance)
    : node(node), distance(distance) {}

void BVH::Octree::insert(OctreeNode*& node, const Extents* extents,
                         const BBox& bbox, uint32_t depth) {
  if (node->isLeaf) {
    if (node->nodeExtentsList.size() == 0 || depth == 16) {
      node->nodeExtentsList.push_back(extents);
    } else {
      node->isLeaf = false;
      // Re-insert extents held by this node
      while (node->nodeExtentsList.size()) {
        insert(node, node->nodeExtentsList.back(), bbox, depth);
        node->nodeExtentsList.pop_back();
      }
      // Insert new extent
      insert(node, extents, bbox, depth);
    }
  } else {
    // Need to compute in which child of the current node this extents should
    // be inserted into
    vec3 extentsCentroid = extents->centroid();
    vec3 nodeCentroid = (bbox[0] + bbox[1]) * 0.5f;
    BBox childBBox;
    uint8_t childIndex = 0;
    // x-axis
    if (extentsCentroid.x > nodeCentroid.x) {
      childIndex = 4;
      childBBox[0].x = nodeCentroid.x;
      childBBox[1].x = bbox[1].x;
    } else {
      childBBox[0].x = bbox[0].x;
      childBBox[1].x = nodeCentroid.x;
    }
    // y-axis
    if (extentsCentroid.y > nodeCentroid.y) {
      childIndex += 2;
      childBBox[0].y = nodeCentroid.y;
      childBBox[1].y = bbox[1].y;
    } else {
      childBBox[0].y = bbox[0].y;
      childBBox[1].y = nodeCentroid.y;
    }
    // z-axis
    if (extentsCentroid.z > nodeCentroid.z) {
      childIndex += 1;
      childBBox[0].z = nodeCentroid.z;
      childBBox[1].z = bbox[1].z;
    } else {
      childBBox[0].z = bbox[0].z;
      childBBox[1].z = nodeCentroid.z;
    }

    // Create the child node if it doesn't exsit yet and then insert the extents
    // in it
    if (node->child[childIndex] == nullptr)
      node->child[childIndex] = new OctreeNode;
    insert(node->child[childIndex], extents, childBBox, depth + 1);
  }
}

void BVH::Octree::build(OctreeNode*& node, const BBox& bbox) {
  if (node->isLeaf) {
    for (const auto& extent : node->nodeExtentsList) {
      node->nodeExtents.extendBy(*extent);
    }
  } else {
    for (uint8_t i = 0; i < 8; ++i) {
      if (node->child[i]) {
        BBox childBBox;
        vec3 centroid = bbox.centroid();
        // x-axis
        childBBox[0].x = (i & 4) ? centroid.x : bbox[0].x;
        childBBox[1].x = (i & 4) ? bbox[1].x : centroid.x;
        // y-axis
        childBBox[0].y = (i & 2) ? centroid.y : bbox[0].y;
        childBBox[1].y = (i & 2) ? bbox[1].y : centroid.y;
        // z-axis
        childBBox[0].z = (i & 1) ? centroid.z : bbox[0].z;
        childBBox[1].z = (i & 1) ? bbox[1].z : centroid.z;

        // Inspect child
        build(node->child[i], childBBox);

        // Expand extents with extents of child
        node->nodeExtents.extendBy(node->child[i]->nodeExtents);
      }
    }
  }
}

/* BVH IMPLEMENTATION */

BVH::BVH(vector<Object*> scene) {
  Extents sceneExtents;
  extentsList.reserve(scene.size());
  for (uint32_t i = 0; i < scene.size(); ++i) {
    for (uint32_t ii = 0; ii < scene[i]->primitives.size(); ++ii) {
      Triangle* tri = dynamic_cast<Triangle*>(scene[i]->primitives[ii]);
      Sphere* sph = dynamic_cast<Sphere*>(scene[i]->primitives[ii]);
      for (uint8_t j = 0; j < normalsSize; ++j) {
        if (tri != nullptr) {
          float v0d = dot(planeSetNormals[j], vec3(tri->v0.position) +
                                                  (planeSetNormals[i] * 1e-4f));
          if (v0d < extentsList[i].slabs[j][0])
            extentsList[i].slabs[j][0] = v0d;
          if (v0d > extentsList[i].slabs[j][1])
            extentsList[i].slabs[j][1] = v0d;

          float v1d = dot(planeSetNormals[j], vec3(tri->v1.position) +
                                                  (planeSetNormals[i] * 1e-4f));
          if (v1d < extentsList[i].slabs[j][0])
            extentsList[i].slabs[j][0] = v1d;
          if (v1d > extentsList[i].slabs[j][1])
            extentsList[i].slabs[j][1] = v1d;

          float v2d = dot(planeSetNormals[j], vec3(tri->v2.position) +
                                                  (planeSetNormals[i] * 1e-4f));
          if (v2d < extentsList[i].slabs[j][0])
            extentsList[i].slabs[j][0] = v2d;
          if (v2d > extentsList[i].slabs[j][1])
            extentsList[i].slabs[j][1] = v2d;
        } else if (sph != nullptr) {
          float p1 = dot(planeSetNormals[j],
                         vec3(sph->c) + (planeSetNormals[j] * sph->radius));
          if (p1 < extentsList[i].slabs[j][0]) extentsList[i].slabs[j][0] = p1;
          if (p1 > extentsList[i].slabs[j][1]) extentsList[i].slabs[j][1] = p1;

          float p2 = dot(planeSetNormals[j],
                         vec3(sph->c) - (planeSetNormals[j] * sph->radius));
          if (p1 < extentsList[i].slabs[j][0]) extentsList[i].slabs[j][0] = p2;
          if (p1 > extentsList[i].slabs[j][1]) extentsList[i].slabs[j][1] = p2;
        }
      }
    }
    sceneExtents.extendBy(extentsList[i]);
    extentsList[i].object = scene[i];
  }

  octree = new Octree(sceneExtents);

  for (uint32_t i = 0; i < scene.size(); ++i) {
    octree->insert(&extentsList[i]);
  }

  octree->build();
}

bool BVH::intersect(Ray ray, Intersection& intersection) const {
  intersection.distance = INFINITY;
  intersection.primitive = nullptr;
  float minDist = INFINITY;
  float precomputedNumerator[normalsSize];
  float precomputedDenominator[normalsSize];
  for (uint8_t i = 0; i < normalsSize; ++i) {
    precomputedNumerator[i] = dot(planeSetNormals[i], vec3(ray.position));
    precomputedDenominator[i] = dot(planeSetNormals[i], vec3(ray.direction));
  }

  uint8_t planeIndex;
  float tNear = 0, tFar = INFINITY;
  if (!octree->root->nodeExtents.intersect(precomputedNumerator,
                                           precomputedDenominator, tNear, tFar,
                                           planeIndex) ||
      tFar < 0)
    return false;
  minDist = tFar;
  std::priority_queue<BVH::Octree::QueueElement> queue;
  queue.push(BVH::Octree::QueueElement(octree->root, 0));
  while (!queue.empty() && queue.top().distance < minDist) {
    const Octree::OctreeNode* node = queue.top().node;
    queue.pop();
    if (node->isLeaf) {
      for (const auto& extent : node->nodeExtentsList) {
        Intersection i;
        if (extent->object->intersect(ray, i)) {
          if (i.distance < minDist) {
            minDist = i.distance;
            intersection = i;
          }
        }
      }
    } else {
      for (uint8_t i = 0; i < 8; ++i) {
        if (node->child[i] != nullptr) {
          float tNearChild = 0, tFarChild = tFar;
          if (node->child[i]->nodeExtents.intersect(
                  precomputedNumerator, precomputedDenominator, tNearChild,
                  tFarChild, planeIndex)) {
            float t =
                (tNearChild < 0 && tFarChild >= 0) ? tFarChild : tNearChild;
            queue.push(BVH::Octree::QueueElement(node->child[i], t));
          }
        }
      }
    }
  }
  return intersection.distance != INFINITY && intersection.primitive != nullptr;
}

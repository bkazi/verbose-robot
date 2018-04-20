#include "bvh.h"
#include <iostream>

using namespace std;
using glm::vec3;


    BBox::BBox() {} 
    BBox::BBox(Vec3<T> min_, Vec3<T> max_) 
    { 
        bounds[0] = min_; 
        bounds[1] = max_; 
    } 
    BBox& BBox::extendBy(const Vec3<T>& p) 
    { 
        if (p.x < bounds[0].x) bounds[0].x = p.x; 
        if (p.y < bounds[0].y) bounds[0].y = p.y; 
        if (p.z < bounds[0].z) bounds[0].z = p.z; 
        if (p.x > bounds[1].x) bounds[1].x = p.x; 
        if (p.y > bounds[1].y) bounds[1].y = p.y; 
        if (p.z > bounds[1].z) bounds[1].z = p.z; 
 
        return *this; 
    } 
    Vec3<T> BBox::centroid() const { return (bounds[0] + bounds[1]) * 0.5; }
 
template<typename T> 
bool BBox<T>::intersect(const Vec3<T>& orig, const Vec3<T>& invDir, const Vec3b& sign, float& tHit) const 
{ 
    numRayBBoxTests++; 
    float tmin, tmax, tymin, tymax, tzmin, tzmax; 
 
    tmin  = (bounds[sign[0]    ].x - orig.x) * invDir.x; 
    tmax  = (bounds[1 - sign[0]].x - orig.x) * invDir.x; 
    tymin = (bounds[sign[1]    ].y - orig.y) * invDir.y; 
    tymax = (bounds[1 - sign[1]].y - orig.y) * invDir.y; 
 
    if ((tmin > tymax) || (tymin > tmax)) 
        return false; 
 
    if (tymin > tmin) 
        tmin = tymin; 
    if (tymax < tmax) 
        tmax = tymax; 
 
    tzmin = (bounds[sign[2]    ].z - orig.z) * invDir.z; 
    tzmax = (bounds[1 - sign[2]].z - orig.z) * invDir.z; 
 
    if ((tmin > tzmax) || (tzmin > tmax)) 
        return false; 
 
    if (tzmin > tmin) 
        tmin = tzmin; 
    if (tzmax < tmax) 
        tmax = tzmax; 
 
    tHit = tmin; 
 
    return true; 
}

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

bool Extents::intersect(Ray *ray, float *precomputedNumerator,
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

Object *BVH::intersect(Ray *ray, vector<Object *> scene) {
  float tClosest = INFINITY;
  Object *hitObject = NULL;
  float precomputedNumerator[normalSize], precomputeDenominator[normalSize];
  for (uint8_t i = 0; i < normalSize; ++i) {
    precomputedNumerator[i] = dot(planeSetNormals[i], vec3(ray->position));
    precomputeDenominator[i] = dot(planeSetNormals[i], vec3(ray->direction));
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

Octree::Octree(const Extents &sceneExtents) {
  float xDiff = sceneExtents.d[0][1] - sceneExtents.d[0][0];
  float yDiff = sceneExtents.d[1][1] - sceneExtents.d[1][0];
  float zDiff = sceneExtents.d[2][1] - sceneExtents.d[2][0];
  float maxDiff = std::max(xDiff, std::max(yDiff, zDiff));
  Vec3f minPlusMax(sceneExtents.d[0][0] + sceneExtents.d[0][1],
                   sceneExtents.d[1][0] + sceneExtents.d[1][1],
                   sceneExtents.d[2][0] + sceneExtents.d[2][1]);
  bbox[0] = (minPlusMax - maxDiff) * 0.5;
  bbox[1] = (minPlusMax + maxDiff) * 0.5;
  root = new OctreeNode;
}

~Octree : Octree() { deleteOctreeNode(root); }

void Octree::insert(const Extents *extents) { insert(root, extents, bbox, 0); }
void Octree::build() { build(root, bbox); };

void Octree::deleteOctreeNode(OctreeNode *&node) {
  for (uint8_t i = 0; i < 8; i++) {
    if (node->child[i] != nullptr) {
      deleteOctreeNode(node->child[i]);
    }
  }
  delete node;
}

void Octree::insert(OctreeNode *&node, const Extents *extents,
                    const BBox<> &bbox, uint32_t depth) {
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
    Vec3f extentsCentroid = extents->centroid();
    Vec3f nodeCentroid = (bbox[0] + bbox[1]) * 0.5;
    BBox<> childBBox;
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

    // Create the child node if it doesn't exsit yet and then insert the
    // extents in it
    if (node->child[childIndex] == nullptr)
      node->child[childIndex] = new OctreeNode;
    insert(node->child[childIndex], extents, childBBox, depth + 1);
  }
}

void Octree::build(OctreeNode *&node, const BBox<> &bbox) {
  if (node->isLeaf) {
    for (const auto &e : node->nodeExtentsList) {
      node->nodeExtents.extendBy(*e);
    }
  } else {
    for (uint8_t i = 0; i < 8; ++i) {
      if (node->child[i]) {
        BBox<> childBBox;
        Vec3f centroid = bbox.centroid();
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
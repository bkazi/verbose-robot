#ifndef BVH_H
#define BVH_H

#include <glm/glm.hpp>
#include <vector>
#include "objects.h"

template<typename T = float> 
class BBox 
{ 
public: 
    BBox() {} 
    BBox(Vec3<T> min_, Vec3<T> max_);
    BBox& extendBy(const Vec3<T>& p);
    Vec3<T> centroid() const;
    Vec3<T>& operator [] (bool i) { return bounds[i]; } 
    const Vec3<T> operator [] (bool i) const { return bounds[i]; } 
    bool intersect(const Vec3<T>&, const Vec3<T>&, const Vec3b&, float&) const; 
    Vec3<T> bounds[2] = { kInfinity, -kInfinity }; 
}; 

struct Extents {
  Extents();
  bool intersect(Ray *ray, float *precomputedNumerator,
                 float *precomputeDenominator, float &tNear, float &tFar,
                 uint8_t &planeIndex);
  float d[7][2];
};

struct BVH {
 private:
  Extents *extents;

 public:
  BVH(std::vector<Object *> scene);
  Object *intersect(Ray *ray, std::vector<Object *> scene);
};

struct Octree {
  Octree(const Extents &sceneExtents);
  ~Octree();

  void insert(const Extents *extents);
  void build();

  struct OctreeNode {
    OctreeNode *child[8] = {nullptr};
    std::vector<const Extents *>
        nodeExtentsList;  // pointer to the objects extents
    Extents nodeExtents;  // extents of the octree node itself
    bool isLeaf = true;
  };

  struct QueueElement {
    const OctreeNode *node;  // octree node held by this element in the queue
    float t;  // distance from the ray origin to the extents of the node
    QueueElement(const OctreeNode *n, float tn) : node(n), t(tn) {}
    // priority_queue behaves like a min-heap
    friend bool operator<(const QueueElement &a, const QueueElement &b) {
      return a.t > b.t;
    }
  };

  OctreeNode *root =
      nullptr;  // make unique son don't have to manage deallocation
  BBox<> bbox;

 private:
  void deleteOctreeNode(OctreeNode *&node) {
    for (uint8_t i = 0; i < 8; i++) {
      if (node->child[i] != nullptr) {
        deleteOctreeNode(node->child[i]);
      }
    }
    delete node;
  }

  void insert(OctreeNode *&node, const Extents *extents, const BBox<> &bbox,
              uint32_t depth);

  void build(OctreeNode *&node, const BBox<> &bbox);
};

#endif
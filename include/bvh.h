#ifndef BVH_H
#define BVH_H

#include <glm/glm.hpp>
#include <queue>
#include <vector>
#include "objects.h"

const uint32_t normalsSize = 7;

class BBox {
 public:
  BBox();
  BBox(glm::vec3 min, glm::vec3 max);
  BBox& extendBy(const glm::vec3& point);
  glm::vec3 centroid() const;
  glm::vec3& operator[](bool i) { return bounds[i]; };
  const glm::vec3 operator[](bool i) const { return bounds[i]; };
  bool intersect(const glm::vec3& start, const glm::vec3& direction,
                 const glm::vec3& sign, float& minDist) const;
  glm::vec3 bounds[2] = {glm::vec3(INFINITY), glm::vec3(-INFINITY)};
};

struct BVH {
  struct Extents {
    Extents();
    void extendBy(const Extents& extents);
    glm::vec3 centroid() const;
    bool intersect(const float* precomputedNumerator,
                   const float* precomputedDenominator, float& nearDist,
                   float& farDist, uint8_t& planeIndex) const;
    float slabs[normalsSize][2];
    const Object* object;
  };

  struct Octree {
    Octree(const Extents& sceneExtents);
    ~Octree();
    void insert(const Extents* extents);
    void build();

    struct OctreeNode {
      OctreeNode* child[8] = {nullptr};
      std::vector<const Extents*> nodeExtentsList;
      Extents nodeExtents;
      bool isLeaf = true;
    };

    struct QueueElement {
      const OctreeNode* node;
      float distance;
      QueueElement(const OctreeNode* node, float distance);
      friend bool operator<(const QueueElement& a, const QueueElement& b) {
        return a.distance > b.distance;
      };
    };

    OctreeNode* root = nullptr;
    BBox bbox;

    void deleteOctreeNode(OctreeNode*& node);
    void insert(OctreeNode*& node, const Extents* extents, const BBox& bbox,
                uint32_t depth);
    void build(OctreeNode*& node, const BBox& bbox);
  };

  std::vector<Extents> extentsList;
  Octree* octree = nullptr;

 public:
  BVH(std::vector<Object*> scene);
  bool intersect(Ray ray, Intersection& intersection) const;
};

#endif
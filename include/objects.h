#ifndef OBJECTS_H
#define OBJECTS_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Primitive; // Forward declare to fix problems

struct Ray {
public:
    glm::vec4 position;
    glm::vec4 direction;
    Ray(glm::vec4 position, glm::vec4 direction);
};

struct Intersection {
  glm::vec4 position;
  float distance;
  Primitive *primitive;
};

struct Primitive {
public:
  glm::vec3 emit;
  glm::vec3 color;
  float shininess;
  float Ka;
  float Ks;
  float Kd;
  float ior;
  bool glass;

  Primitive(glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks,
        float Kd, float ior, bool glass);
  virtual float intersect(Ray *ray);
  virtual glm::vec4 randomPoint();
  virtual glm::vec4 getNormal(const glm::vec4 &p);
  virtual bool isLight();
};

struct Object {
 public:
  Object(std::vector<Primitive *> primitives);
  void computeBounds(const glm::vec3 &planeNormal, float &dnear, float &dfar);
  std::vector<Primitive *> primitives;
};

class Triangle : public Primitive {
 public:
  glm::vec4 v0;
  glm::vec4 v1;
  glm::vec4 v2;
  glm::vec3 e1;
  glm::vec3 e2;

  Triangle(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, glm::vec3 emit,
           glm::vec3 color, float shininess, float Ka, float Ks, float Kd, float ior, bool glass);

  glm::vec4 getNormal(const glm::vec4 &p = glm::vec4(0)) override;

  glm::vec4 randomPoint() override;

  float intersect(Ray *ray) override;

  void ComputeNormal();

 private:
  glm::vec4 normal;
};

class Sphere : public Primitive {
 public:
  glm::vec4 c;
  float radius;

  Sphere(glm::vec4 c, float radius, glm::vec3 emit, glm::vec3 color,
         float shininess, float Ka, float Ks, float Kd, float ior, bool glass);

  glm::vec4 getNormal(const glm::vec4 &p) override;

  float intersect(Ray *ray) override;
};

#endif

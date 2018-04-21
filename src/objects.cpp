#include "objects.h"
#include <iostream>

using namespace std;

using glm::determinant;
using glm::dot;
using glm::mat3;
using glm::vec2;
using glm::vec3;
using glm::vec4;

Ray::Ray(vec4 position, vec4 direction)
    : position(position), direction(direction) {}

/* OBJECT CLASS IMPLEMENTATION */
Object::Object(vector<Primitive *> primitives) : primitives(primitives){};
void Object::computeBounds(const vec3 &planeNormal, float &dnear, float &dfar) {
  float d;
  for (uint32_t i = 0; i < primitives.size(); i++) {
    Triangle *tri;
    Sphere *sph;
    if ((tri = dynamic_cast<Triangle *>(primitives[i]))) {
      d = dot(planeNormal, vec3(tri->v0.x, tri->v0.y, tri->v0.z));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;

      d = dot(planeNormal, vec3(tri->v1.x, tri->v1.y, tri->v1.z));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;

      d = dot(planeNormal, vec3(tri->v2.x, tri->v2.y, tri->v2.z));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;
    }
    if ((sph = dynamic_cast<Sphere *>(primitives[i]))) {
      d = dot(planeNormal,
              vec3(sph->c.x, sph->c.y, sph->c.z) + (planeNormal * sph->radius));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;

      d = dot(planeNormal,
              vec3(sph->c.x, sph->c.y, sph->c.z) - (planeNormal * sph->radius));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;
    }
  }
}

/* SHAPE CLASS IMPLEMENTATION */
Primitive::Primitive(vec3 emit, vec3 color, float shininess, float Ka, float Ks,
                     float Kd)
    : emit(emit), color(color), shininess(shininess), Ka(Ka), Ks(Ks), Kd(Kd) {}
vec4 Primitive::randomPoint() { return vec4(); };
vec4 Primitive::getNormal(const vec4 &p) { return vec4(); };
bool Primitive::isLight() { return emit.x > 0 || emit.y > 0 || emit.z > 0; }
float Primitive::intersect(Ray *ray) { return INFINITY; }

/* TRIANGLE CLASS IMPLEMENTATION */
Triangle::Triangle(vec4 v0, vec4 v1, vec4 v2, vec3 emit, vec3 color,
                   float shininess, float Ka, float Ks, float Kd)
    : Primitive(emit, color, shininess, Ka, Ks, Kd), v0(v0), v1(v1), v2(v2) {
  Triangle::ComputeNormal();
}
vec4 Triangle::getNormal(const vec4 &p) { return normal; }
vec4 Triangle::randomPoint() {
  while (1) {
    float u = rand() / (float)RAND_MAX;
    float v = rand() / (float)RAND_MAX;
    if (u >= 0 && v >= 0 && u + v <= 1) {
      return v0 + vec4(u * e1 + v * e2, 1);
    }
  }
}
float Triangle::intersect(Ray *ray) {
  vec3 b = glm::vec3(ray->position - v0);
  mat3 A(-glm::vec3(ray->direction), e1, e2);
  float detA = determinant(A);
  float dist = determinant(mat3(b, e1, e2)) / detA;

  // Calculate point of intersection
  if (dist > 0) {
    float u = determinant(mat3(-vec3(ray->direction), b, e2)) / detA;
    float v = determinant(mat3(-vec3(ray->direction), e1, b)) / detA;
    if (u >= 0 && v >= 0 && u + v <= 1) {
      return dist;
    }
  }
  return INFINITY;
}
void Triangle::ComputeNormal() {
  e1 = glm::vec3(v1 - v0);
  e2 = glm::vec3(v2 - v0);
  glm::vec3 normal3 = glm::normalize(glm::cross(e2, e1));
  normal = glm::vec4(normal3, 1.0f);
}

/* SPHERE CLASS IMPLEMENTATION */
Sphere::Sphere(vec4 c, float radius, vec3 emit, vec3 color, float shininess,
               float Ka, float Ks, float Kd)
    : Primitive(emit, color, shininess, Ka, Ks, Kd), c(c), radius(radius) {}
vec4 Sphere::getNormal(const vec4 &p) { return (p - c) / radius; }
float Sphere::intersect(Ray *ray) {
  vec4 sC = ray->position - c;
  float inSqrt =
      powf(radius, 2.0f) - (dot(sC, sC) - powf(dot(ray->direction, sC), 2.0f));
  float dist, dist1, dist2;
  if (inSqrt < 0) {
    return INFINITY;
  } else if (inSqrt == 0) {
    dist = -dot(ray->direction, sC);
  } else {
    dist1 = -dot(ray->direction, sC) + sqrt(inSqrt);
    dist2 = -dot(ray->direction, sC) - sqrt(inSqrt);
    dist = dist1 > 0 && dist1 < dist2 ? dist1 : dist2;
  }
  if (dist > 0) {
    return dist;
  }
  return INFINITY;
}

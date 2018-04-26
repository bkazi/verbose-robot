#include "objects.h"
#include <iostream>

using namespace std;

using cv::Mat;
using cv::Vec3b;
using cv::Point;
using glm::determinant;
using glm::dot;
using glm::clamp;
using glm::mat3;
using glm::vec2;
using glm::vec3;
using glm::vec4;

/* TEXTURE IMPLEMENTATION */
std::map<std::string, Texture *> Texture::textures;
Texture::Texture(Mat image) : image(image){};
Texture *Texture::createTexture(string path) {
  cout << "Looking for texture in: " << path << endl;
  if (Texture::textures.find(path) != Texture::textures.end()) {
    return Texture::textures[path];
  } else {
    Mat image;
    image = cv::imread(path, 1);
    Texture *texture = new Texture(image);
    Texture::textures[path] = texture;
    return texture;
  }
}
vec3 Texture::sample(vec2 uv) {
  int u = round(clamp(uv.x * image.cols, 0.f, (float) image.cols));
  int v = round(clamp(uv.y * image.rows, 0.f, (float) image.rows));

  Vec3b color = image.at<Vec3b>(Point(u, v));
  float b = clamp((float)color[0] / 255.f, 0.f, 1.f);
  float g = clamp((float)color[1] / 255.f, 0.f, 1.f);
  float r = clamp((float)color[2] / 255.f, 0.f, 1.f);
  return vec3(r, g, b);
}

/* VERTEX IMPLEMENTATION */
Vertex::Vertex(){};
Vertex::Vertex(vec4 position) : position(position){};
Vertex::Vertex(vec4 position, vec4 normal, vec3 color)
    : position(position), normal(normal), color(color){};
Vertex::Vertex(vec4 position, vec4 normal, vec2 uv, vec3 color)
    : position(position), normal(normal), uv(uv), color(color){};

/* SHAPE CLASS IMPLEMENTATION */
Primitive::Primitive(Material material) : material(material) {}
vec4 Primitive::randomPoint() { return vec4(); };
vec4 Primitive::getNormal(const vec4 &p) { return vec4(); };
bool Primitive::isLight() {
  return material.emission.x > 0 || material.emission.y > 0 ||
         material.emission.z > 0;
}
float Primitive::intersect(Ray ray) { return INFINITY; }

/* OBJECT CLASS IMPLEMENTATION */
Object::Object(vector<Primitive *> primitives) : primitives(primitives){};
void Object::computeBounds(const vec3 &planeNormal, float &dnear, float &dfar) {
  float d;
  for (uint32_t i = 0; i < primitives.size(); i++) {
    Triangle *tri;
    Sphere *sph;
    if ((tri = dynamic_cast<Triangle *>(primitives[i]))) {
      d = dot(planeNormal,
              vec3(tri->v0.position.x, tri->v0.position.y, tri->v0.position.z));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;

      d = dot(planeNormal,
              vec3(tri->v1.position.x, tri->v1.position.y, tri->v1.position.z));
      if (d < dnear) dnear = d;
      if (d > dfar) dfar = d;

      d = dot(planeNormal,
              vec3(tri->v2.position.x, tri->v2.position.y, tri->v2.position.z));
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

/* TRIANGLE CLASS IMPLEMENTATION */
Triangle::Triangle(Vertex vertex0, Vertex vertex1, Vertex vertex2, Material material)
    : Primitive(material), v0(vertex0), v1(vertex1), v2(vertex2) {
  Triangle::ComputeNormal();
  v0.color = material.color;
  v1.color = material.color;
  v2.color = material.color;
}
vec4 Triangle::getNormal(const vec4 &p) { return normal; }
vec4 Triangle::randomPoint() {
  while (1) {
    float u = rand() / (float)RAND_MAX;
    float v = rand() / (float)RAND_MAX;
    if (u >= 0 && v >= 0 && u + v <= 1) {
      return v0.position + vec4(u * e1 + v * e2, 1);
    }
  }
}
float Triangle::intersect(Ray ray) {
  vec3 b = glm::vec3(ray.position - v0.position);
  mat3 A(-glm::vec3(ray.direction), e1, e2);
  float detA = determinant(A);
  float dist = determinant(mat3(b, e1, e2)) / detA;

  // Calculate point of intersection
  if (dist > 0) {
    float u = determinant(mat3(-vec3(ray.direction), b, e2)) / detA;
    float v = determinant(mat3(-vec3(ray.direction), e1, b)) / detA;
    if (u >= 0 && v >= 0 && u + v <= 1) {
      return dist;
    }
  }
  return INFINITY;
}
void Triangle::ComputeNormal() {
  e1 = glm::vec3(v1.position - v0.position);
  e2 = glm::vec3(v2.position - v0.position);
  glm::vec3 normal3 = glm::normalize(glm::cross(e2, e1));
  normal = glm::vec4(normal3, 0.0f);
}

/* SPHERE CLASS IMPLEMENTATION */
Sphere::Sphere(vec4 c, float radius, Material material)
    : Primitive(material), c(c), radius(radius) {}
vec4 Sphere::getNormal(const vec4 &p) { return (p - c) / radius; }
float Sphere::intersect(Ray ray) {
  vec4 sC = ray.position - c;
  float inSqrt =
      powf(radius, 2.0f) - (dot(sC, sC) - powf(dot(ray.direction, sC), 2.0f));
  float dist, dist1, dist2;
  if (inSqrt < 0) {
    return INFINITY;
  } else if (inSqrt == 0) {
    dist = -dot(ray.direction, sC);
  } else {
    dist1 = -dot(ray.direction, sC) + sqrt(inSqrt);
    dist2 = -dot(ray.direction, sC) - sqrt(inSqrt);
    dist = dist1 > 0 && dist1 < dist2 ? dist1 : dist2;
  }
  if (dist > 0) {
    return dist;
  }
  return INFINITY;
}

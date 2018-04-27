#ifndef OBJECTS_H
#define OBJECTS_H

#include <glm/glm.hpp>
#include <map>
#include <opencv/cv.hpp>
#include <string>
#include <vector>

struct Primitive;  // Forward declare to fix problems

struct Ray {
  glm::vec4 position;
  glm::vec4 direction;
};

struct Intersection {
  glm::vec4 position;
  float distance;
  Primitive *primitive;
};

struct Texture {
  static Texture *createTexture(std::string);
  cv::Mat image;
  Texture(cv::Mat image);
  glm::vec3 sample(glm::vec2);
  static std::map<std::string, Texture *> textures;
};

struct Material {
  Material()
      : color(glm::vec3(0)),
        ambient(glm::vec3(0)),
        diffuse(glm::vec3(0)),
        specular(glm::vec3(0)),
        transmittance(glm::vec3(0)),
        emission(glm::vec3(0)),
        shininess(1.f),
        refractiveIndex(1.f),
        dissolve(0.f),
        ambientTexture(nullptr),
        diffuseTexture(nullptr),
        specularTexture(nullptr),
        specularHighlightTexture(nullptr),
        bumpTexture(nullptr),
        displacementTexture(nullptr),
        alphaTexture(nullptr),
        reflectionTexture(nullptr) {}
  glm::vec3 color;
  glm::vec3 ambient;
  glm::vec3 diffuse;
  glm::vec3 specular;
  glm::vec3 transmittance;
  glm::vec3 emission;
  float shininess;
  float refractiveIndex;
  float dissolve;
  Texture *ambientTexture;
  Texture *diffuseTexture;
  Texture *specularTexture;
  Texture *specularHighlightTexture;
  Texture *bumpTexture;
  Texture *displacementTexture;
  Texture *alphaTexture;
  Texture *reflectionTexture;
};

struct Vertex {
  glm::vec4 position;
  glm::vec4 normal;
  glm::vec2 uv;
  glm::vec3 color;

  Vertex();
  Vertex(glm::vec4 position);
  Vertex(glm::vec4 position, glm::vec4 normal, glm::vec2 uv, glm::vec3 color);
  Vertex(glm::vec4 position, glm::vec4 normal, glm::vec3 color);
};

struct Primitive {
 public:
  Material material;

  Primitive(Material material);
  virtual float intersect(Ray ray);
  virtual glm::vec4 randomPoint();
  virtual glm::vec4 getNormal(const glm::vec4 &p);
  virtual bool isLight();
};

struct Object {
 public:
  Object(std::vector<Primitive *> primitives);
  void computeBounds(const glm::vec3 &planeNormal, float &dnear, float &dfar);
  bool intersect(Ray ray, Intersection &intersection) const;
  std::vector<Primitive *> primitives;
};

class Triangle : public Primitive {
 public:
  Vertex v0;
  Vertex v1;
  Vertex v2;
  glm::vec3 e1;
  glm::vec3 e2;

  Triangle(Vertex v0, Vertex v1, Vertex v2, Material material);
  glm::vec4 getNormal(const glm::vec4 &p = glm::vec4(0)) override;
  glm::vec4 randomPoint() override;
  float intersect(Ray ray) override;
  void ComputeNormal();

 private:
  glm::vec4 normal;
};

class Sphere : public Primitive {
 public:
  glm::vec4 c;
  float radius;

  Sphere(glm::vec4 c, float radius, Material material);
  glm::vec4 getNormal(const glm::vec4 &p) override;
  float intersect(Ray ray) override;
};

#endif

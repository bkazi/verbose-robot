#ifndef SDL_AUXILIARY_H
#define SDL_AUXILIARY_H

#include <glm/glm.hpp>

struct Intersection {
  glm::vec4 position;
  float distance;
  int shapeIndex;
};

struct Shape {
    glm::vec3 emit;
    glm::vec3 color;
    float shininess;
    float Ka;
    float Ks;
    float Kd;

    Shape(glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks, float Kd) : emit(emit), color(color), shininess(shininess), Ka(Ka), Ks(Ks), Kd(Kd) {};
    virtual float intersects(const glm::vec4 start, const glm::vec4 direction);
    virtual glm::vec4 randomPoint();
    virtual glm::vec4 getNormal(const glm::vec4 &p);
    virtual bool isLight();
};

class Triangle : public Shape {
public:
	glm::vec4 v0;
	glm::vec4 v1;
	glm::vec4 v2;
	glm::vec3 e1;
	glm::vec3 e2;

	Triangle(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks, float Kd) : Shape(emit, color, shininess, Ka, Ks, Kd), v0(v0), v1(v1), v2(v2) {};

    glm::vec4 getNormal(const glm::vec4 &p);

    glm::vec4 randomPoint();

    float intersects(const glm::vec4 start, const glm::vec4 direction);

	void ComputeNormal();
};

class Sphere : public Shape {
public:
	glm::vec4 c;
	float radius;

	Sphere(glm::vec4 c, float radius, glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks, float Kd) : Shape(emit, color, shininess, Ka, Ks, Kd), c(c), radius(radius) {};

    glm::vec4 getNormal(const glm::vec4 &p);

    float intersects(const glm::vec4 start, const glm::vec4 direction);
};

#endif

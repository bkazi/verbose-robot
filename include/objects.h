#ifndef OBJECTS_H
#define OBJECTS_H

#include <glm/glm.hpp>
#include <vector>
#include <string>

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

    Shape(glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks, float Kd);
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

	Triangle(glm::vec4 v0, glm::vec4 v1, glm::vec4 v2, glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks, float Kd);

    glm::vec4 getNormal(const glm::vec4 &p);

    glm::vec4 randomPoint();

    float intersects(const glm::vec4 start, const glm::vec4 direction);

	void ComputeNormal();

private:
	glm::vec4 normal;
};

class Sphere : public Shape {
public:
	glm::vec4 c;
	float radius;

	Sphere(glm::vec4 c, float radius, glm::vec3 emit, glm::vec3 color, float shininess, float Ka, float Ks, float Kd);

    glm::vec4 getNormal(const glm::vec4 &p);

    float intersects(const glm::vec4 start, const glm::vec4 direction);
};

#endif

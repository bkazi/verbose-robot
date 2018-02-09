#include "objects.h"

using glm::mat3;
using glm::vec3;
using glm::vec4;
using glm::determinant;

struct Intersection {
  vec4 position;
  float distance;
  int shapeIndex;
};

struct Shape {
    vec3 emit;
    vec3 color;
    float shininess;
    float Ka;
    float Ks;
    float Kd;

    Shape(vec3 emit, vec3 color, float shininess, float Ka, float Ks, float Kd) : emit(emit), color(color), shininess(shininess), Ka(Ka), Ks(Ks), Kd(Kd) {}
    virtual float intersects(const vec4 start, const vec4 direction) {
        return -1;
    }
    virtual vec4 randomPoint() {return vec4();};
    virtual vec4 getNormal(const vec4 &p) {return vec4();};
    virtual bool isLight() {return emit.x > 0 || emit.y > 0 || emit.z > 0;}
};

class Triangle : public Shape {
public:
	vec4 v0;
	vec4 v1;
	vec4 v2;
	vec3 e1;
	vec3 e2;

	Triangle(vec4 v0, vec4 v1, vec4 v2, vec3 emit, vec3 color, float shininess, float Ka, float Ks, float Kd) : Shape(emit, color, shininess, Ka, Ks, Kd), v0(v0), v1(v1), v2(v2) {
		ComputeNormal();
	}

    vec4 getNormal(const vec4 &p) {
        return normal;
    }

    vec4 randomPoint() {
        while (1) {
            float u = rand() / (float) RAND_MAX;
            float v = rand() / (float) RAND_MAX;
            if (u >= 0 && v >= 0 && u + v <= 1) {
                return v0 + vec4(u * e1 + v * e2, 1);
            }
        }
    }

    float intersects(const vec4 start, const vec4 direction) {
        vec3 b = glm::vec3(start - v0);
        mat3 A(-glm::vec3(direction), e1, e2);
        float detA = determinant(A);
        float dist = determinant(mat3(b, e1, e2)) / detA;
        
        // Calculate point of intersection
        if (dist > 0) {
            float u = determinant(mat3(-vec3(direction), b, e2)) / detA;
            float v = determinant(mat3(-vec3(direction), e1, b)) / detA;
            if (u >= 0 && v >= 0 && u + v <= 1) {
                return dist;
            }
        }
        return -1;
    }

	void ComputeNormal() {
	  e1 = glm::vec3(v1 - v0);
	  e2 = glm::vec3(v2 - v0);
	  glm::vec3 normal3 = glm::normalize(glm::cross(e2, e1));
	  normal = glm::vec4(normal3, 1.0f);
	}

private:
	glm::vec4 normal;

};

class Sphere : public Shape {
public:
	vec4 c;
	float radius;

	Sphere(vec4 c, float radius, vec3 emit, vec3 color, float shininess, float Ka, float Ks, float Kd) : Shape(emit, color, shininess, Ka, Ks, Kd), c(c), radius(radius) {}

    vec4 getNormal(const vec4 &p) {
        return (p - c) / radius; 
    }

    float intersects(const vec4 start, const vec4 direction) {
        vec4 sC = start - c;
        float inSqrt =  powf(radius, 2.0f) - (dot(sC, sC) - powf(dot(direction, sC), 2.0f));
        float dist, dist1, dist2;
        if (inSqrt < 0) {
            return -1;
        }
        else if (inSqrt == 0) {
            dist = -dot(direction, sC);
        } else {
            dist1 = -dot(direction, sC) + sqrt(inSqrt);
            dist2 = -dot(direction, sC) - sqrt(inSqrt);
            dist = dist1 > 0 && dist1 < dist2 ? dist1 : dist2;
        }
        if (dist > 0) {
            return dist;
        }
        return -1;
    }
};
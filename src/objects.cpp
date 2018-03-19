#include "objects.h"

using namespace std;

using glm::mat3;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::determinant;
using glm::dot;
using tinyobj::attrib_t;
using tinyobj::shape_t;
using tinyobj::material_t;
using tinyobj::real_t;
using tinyobj::index_t;

/* SHAPE CLASS IMPLEMENTATION */
Shape::Shape(vec3 emit, vec3 color, float shininess, float Ka, float Ks, float Kd) : emit(emit), color(color), shininess(shininess), Ka(Ka), Ks(Ks), Kd(Kd) {}
float Shape::intersects(const vec4 start, const vec4 direction) {
    return -1;
}
vec4 Shape::randomPoint() {
    return vec4();
};
vec4 Shape::getNormal(const vec4 &p) {
    return vec4();
};
bool Shape::isLight() {
    return emit.x > 0 || emit.y > 0 || emit.z > 0;
}

/* TRIANGLE CLASS IMPLEMENTATION */
Triangle::Triangle(vec4 v0, vec4 v1, vec4 v2, vec3 emit, vec3 color, float shininess, float Ka, float Ks, float Kd) : Shape(emit, color, shininess, Ka, Ks, Kd), v0(v0), v1(v1), v2(v2) {
    Triangle::ComputeNormal();
}
vec4 Triangle::getNormal(const vec4 &p) {
    return normal;
}
vec4 Triangle::randomPoint() {
    while (1) {
        float u = rand() / (float) RAND_MAX;
        float v = rand() / (float) RAND_MAX;
        if (u >= 0 && v >= 0 && u + v <= 1) {
            return v0 + vec4(u * e1 + v * e2, 1);
        }
    }
}
float Triangle::intersects(const vec4 start, const vec4 direction) {
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
void Triangle::ComputeNormal() {
    e1 = glm::vec3(v1 - v0);
    e2 = glm::vec3(v2 - v0);
    glm::vec3 normal3 = glm::normalize(glm::cross(e2, e1));
    normal = glm::vec4(normal3, 1.0f);
}

/* SPHERE CLASS IMPLEMENTATION */
Sphere::Sphere(vec4 c, float radius, vec3 emit, vec3 color, float shininess, float Ka, float Ks, float Kd) : Shape(emit, color, shininess, Ka, Ks, Kd), c(c), radius(radius) {}
vec4 Sphere::getNormal(const vec4 &p) {
    return (p - c) / radius; 
}
float Sphere::intersects(const vec4 start, const vec4 direction) {
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

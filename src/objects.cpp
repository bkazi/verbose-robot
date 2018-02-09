#include "objects.h"
#include "tiny_obj_loader.h"

using glm::mat3;
using glm::vec3;
using glm::vec4;
using glm::determinant;
using glm:dot;
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

/* Load Model into scene */
void LoadModel(vector<Shape *> &scene, const char *path) {
  attrib_t attrib;
  vector<shape_t> shapes;
  vector<material_t> materials;
  std::string error;
  std::string pathString = static_cast<string>(path);
  // NB: Lib automatically triangulises -- can be disabled, but is default true
  bool ret = tinyobj::LoadObj(
    &attrib,
    &shapes,
    &materials,
    &error,
    path,
    pathString.substr(0, pathString.find_last_of('/') + 1).c_str()
  );
  if (!error.empty()) {
    cerr << error << endl;
  }
  if (!ret) {
    exit(1);
  }

  // For each shape?
  for (size_t s = 0; s < shapes.size(); s++) {
    size_t index_offset = 0;

    // For each face
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      int fv = shapes[s].mesh.num_face_vertices[f];

      vector<vec4> verticies;
      vector<vec4> normals;
      vector<vec2> textures;
      vector<vec3> colours;

      // For each vertex
      for (size_t v = 0; v < fv; v++) {

        // access to vertex
        index_t idx = shapes[s].mesh.indices[index_offset + v];

        real_t vx = attrib.vertices[3*idx.vertex_index+0];
        real_t vy = attrib.vertices[3*idx.vertex_index+1];
        real_t vz = attrib.vertices[3*idx.vertex_index+2];
        verticies.push_back(vec4(vx, vy, vz, 1));

        real_t nx = attrib.normals[3*idx.normal_index+0];
        real_t ny = attrib.normals[3*idx.normal_index+1];
        real_t nz = attrib.normals[3*idx.normal_index+2];
        normals.push_back(vec4(nx, ny, nz, 1));

        real_t tx = attrib.texcoords[2*idx.texcoord_index+0];
        real_t ty = attrib.texcoords[2*idx.texcoord_index+1];
        textures.push_back(vec2(tx, ty));

        // Optional: vertex colors
        real_t red = attrib.colors[3*idx.vertex_index+0];
        real_t green = attrib.colors[3*idx.vertex_index+1];
        real_t blue = attrib.colors[3*idx.vertex_index+2];
        colours.push_back(vec3(red, green, blue));
      }
      index_offset += fv;

      // per-face material
      tinyobj::material_t material = materials[shapes[s].mesh.material_ids[f]];

      scene.push_back(new Triangle(
        verticies[0],
        verticies[2],
        verticies[1],
        vec3(material.emission[0], material.emission[1], material.emission[2]),
        vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]),
        (float)material.shininess,
        dot(vec3(1), vec3(material.ambient[0], material.ambient[1], material.ambient[2])) / 3.0f,
        dot(vec3(1), vec3(material.specular[0], material.specular[1], material.specular[2])) / 3.0f,
        dot(vec3(1), vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2])) / 3.0f
      )); 
    }
  }
}

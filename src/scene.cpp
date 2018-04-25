#include <iostream>

#include "TestModel.h"
#include "bvh.h"
#include "scene.h"
#include "tiny_obj_loader.h"

#include <glm/gtx/string_cast.hpp>

using namespace std;
using glm::dot;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using tinyobj::attrib_t;
using tinyobj::index_t;
using tinyobj::material_t;
using tinyobj::real_t;
using tinyobj::shape_t;

Scene::Scene() {
  vector<Object *> o;
  objects = o;
}

Scene::Scene(vector<Object *> objects) : objects(objects) {}

bool Scene::intersect(Ray ray, Intersection &intersection) {
  Primitive *closestPrimitive = NULL;
  float minDist = INFINITY;
  if (bvh != NULL) {
    Object *object = bvh->intersect(ray, objects);

    if (object != NULL) {
      float minDist = INFINITY;

      for (uint32_t i = 0; i < object->primitives.size(); ++i) {
        float dist = object->primitives[i]->intersect(ray);
        if (dist < minDist) {
          minDist = dist;
          closestPrimitive = object->primitives[i];
        }
      }
    }
  } else {
    for (uint32_t i = 0; i < objects.size(); ++i) {
      for (uint32_t j = 0; j < objects[i]->primitives.size(); ++j) {
        float dist = objects[i]->primitives[j]->intersect(ray);
        if (dist < minDist) {
          minDist = dist;
          closestPrimitive = objects[i]->primitives[j];
        }
      }
    }
  }
  intersection.primitive = closestPrimitive;
  intersection.distance = minDist;
  intersection.position = ray.position + minDist * ray.direction;
  return closestPrimitive != NULL || minDist != INFINITY;
}

void Scene::createBVH() { bvh = new BVH(objects); }

void Scene::LoadTest() { LoadTestModel(objects); }

/* Load Model into scene */
void Scene::LoadModel(string path) {
  attrib_t attrib;
  vector<shape_t> shapes;
  vector<material_t> materials;
  string error;

  // NB: Lib automatically triangulises -- can be disabled, but is default true
  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &error, path.c_str(),
                       path.substr(0, path.find_last_of('/') + 1).c_str());

  if (!error.empty()) {
    cerr << error << endl;
  }

  if (!ret) {
    exit(1);
  }

  vector<Primitive *> primitives;

  // For each shape?
  for (size_t s = 0; s < shapes.size(); s++) {
    primitives.clear();
    size_t index_offset = 0;

    // For each face
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      // per-face material
      tinyobj::material_t material = materials[shapes[s].mesh.material_ids[f]];

      bool textured =
          material.ambient_texname != "" || material.diffuse_texname != "" ||
          material.specular_texname != "" ||
          material.specular_highlight_texname != "" ||
          material.bump_texname != "" || material.displacement_texname != "" ||
          material.alpha_texname != "" || material.reflection_texname != "";

      int fv = shapes[s].mesh.num_face_vertices[f];

      vector<Vertex> vertices;

      // For each vertex
      for (size_t v = 0; v < fv; v++) {
        // access to vertex
        index_t idx = shapes[s].mesh.indices[index_offset + v];

        real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
        real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
        real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

        real_t nx = attrib.normals[3 * idx.normal_index + 0];
        real_t ny = attrib.normals[3 * idx.normal_index + 1];
        real_t nz = attrib.normals[3 * idx.normal_index + 2];

        real_t red = attrib.colors[3 * idx.vertex_index + 0];
        real_t green = attrib.colors[3 * idx.vertex_index + 1];
        real_t blue = attrib.colors[3 * idx.vertex_index + 2];

        if (textured) {
          real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
          real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
          Vertex vertex = Vertex(vec4(vx, vy, vz, 1), vec4(nx, ny, nz, 0),
                                 vec2(tx, ty), vec3(red, green, blue));
          vertices.push_back(vertex);
        } else {
          Vertex vertex = Vertex(vec4(vx, vy, vz, 1), vec4(nx, ny, nz, 0),
                                 vec3(red, green, blue));
          vertices.push_back(vertex);
        }
      }
      index_offset += fv;

      if (textured) {
        TexturedMaterial mat;
        mat.color =
            vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
        mat.ambient =
            vec3(material.ambient[0], material.ambient[1], material.ambient[2]);
        mat.diffuse =
            vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
        mat.specular = vec3(material.specular[0], material.specular[1],
                            material.specular[2]);
        mat.transmittance =
            vec3(material.transmittance[0], material.transmittance[1],
                 material.transmittance[2]);
        mat.emission = vec3(material.emission[0], material.emission[1],
                            material.emission[2]);
        mat.shininess = material.shininess;
        mat.refractiveIndex = material.ior;
        mat.dissolve = material.dissolve;

        if (material.ambient_texname != "") {
          mat.ambientTexture = Texture::createTexture(material.ambient_texname);
        }
        if (material.diffuse_texname != "") {
          mat.diffuseTexture = Texture::createTexture(material.diffuse_texname);
        }
        if (material.specular_texname != "") {
          mat.specularTexture =
              Texture::createTexture(material.specular_texname);
        }
        if (material.specular_highlight_texname != "") {
          mat.specularHighlightTexture =
              Texture::createTexture(material.specular_highlight_texname);
        }
        if (material.bump_texname != "") {
          mat.bumpTexture = Texture::createTexture(material.bump_texname);
        }
        if (material.displacement_texname != "") {
          mat.displacementTexture =
              Texture::createTexture(material.displacement_texname);
        }
        if (material.alpha_texname != "") {
          mat.alphaTexture = Texture::createTexture(material.alpha_texname);
        }
        if (material.reflection_texname != "") {
          mat.reflectionTexture =
              Texture::createTexture(material.reflection_texname);
        }

        primitives.push_back(
            new Triangle(vertices[0], vertices[2], vertices[1], mat));
      } else {
        Material mat;
        mat.color =
            vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
        mat.ambient =
            vec3(material.ambient[0], material.ambient[1], material.ambient[2]);
        mat.diffuse =
            vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]);
        mat.specular = vec3(material.specular[0], material.specular[1],
                            material.specular[2]);
        mat.transmittance =
            vec3(material.transmittance[0], material.transmittance[1],
                 material.transmittance[2]);
        mat.emission = vec3(material.emission[0], material.emission[1],
                            material.emission[2]);
        mat.shininess = material.shininess;
        mat.refractiveIndex = material.ior;
        mat.dissolve = material.dissolve;
        primitives.push_back(
            new Triangle(vertices[0], vertices[2], vertices[1], mat));
      }
    }

    objects.push_back(new Object(primitives));
  }
}

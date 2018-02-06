#include <iostream>
#include <stdlib.h>
#include <random> 
#include <time.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include "tiny_obj_loader.h"

using namespace std;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::dot;
using tinyobj::attrib_t;
using tinyobj::shape_t;
using tinyobj::material_t;
using tinyobj::real_t;
using tinyobj::index_t;

#define TINYOBJLOADER_IMPLEMENTATION
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false
#define NUM_RAYS 0
#define BOUNCES 3
#define MIN_BOUNCES 4
#define MAX_BOUNCES 10
#define NUM_SAMPLES 2
#define LIVE

float m = numeric_limits<float>::max();
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1, 1, 1); 
vec3 indirectLighting = 0.5f * vec3(1, 1, 1);
float apertureSize = 0.1;

/* ----------------------------------------------------------------------------*/
/* STRUCTS                                                                     */
struct Camera {
  float focalLength;
  vec4 position;
  mat3 R;
  vec3 rotation;
  vec3 movement;
  float movementSpeed;
  float rotationSpeed;
};

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update(Camera &camera, screen *screen);
void Draw(screen *screen, Camera camera);
bool ClosestIntersection(vec4 start, vec4 dir, Intersection &closestIntersection);
vec3 DirectLight(const Intersection &intersection, vec4 dir, bool spec);
vec3 IndirectLight(const Intersection &intersection, vec4 dir, int bounce, bool spec);
mat3 CalcRotationMatrix(float x, float y, float z);
vec3 uniformSampleHemisphere(const float &r1, const float &r2);
void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb);
vec3 Light(const vec4 start, const vec4 dir, int bounce);
float max3(vec3);
void LoadModel(vector<Shape *> &scene, const char *path);

float samples = 0;
vector<Shape *> shapes;
int main(int argc, char *argv[]) {
  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  LoadTestModel(shapes);
  const string path = argv[1];
  LoadModel(shapes, path.c_str());

  Camera camera = {
    SCREEN_HEIGHT,
    vec4(0, 0, -3, 1),
    mat3(1),
    vec3(0, 0, 0),
    vec3(0, 0, 0),
    0.001,
    0.001,
  };

  srand(42);
  memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
#ifdef LIVE
  while (NoQuitMessageSDL()) {
    Update(camera, screen);
    Draw(screen, camera);
    SDL_Renderframe(screen);
  }
#else
  Update(camera, screen);
  Draw(screen, camera);
  SDL_Renderframe(screen);
  Update(camera, screen);
#endif

  SDL_SaveImage(screen, "screenshot.bmp");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen *screen, Camera camera) {
  samples++;

  float depth[SCREEN_WIDTH][SCREEN_HEIGHT];
  float tempDepth;
  float maxDepth = -1;

  Intersection intersection;
  int x, y;
  #pragma omp parallel for private(x, y, intersection, tempDepth, maxDepth)
  for (y = -SCREEN_HEIGHT/2; y < SCREEN_HEIGHT/2; y++) {
    for (x = -SCREEN_WIDTH/2; x < SCREEN_WIDTH/2; x++) {
      vec3 color = vec3(0);
      tempDepth = 0;
#if NUM_RAYS <= 1
      vec4 direction = glm::normalize(vec4(vec3(x, y, camera.focalLength) * camera.R, 1));
      color += Light(camera.position, direction, 0);
      tempDepth += intersection.distance;
#else
      for (int i = -NUM_RAYS/2; i < NUM_RAYS/2; i++) {
        for (int j = -NUM_RAYS/2; j < NUM_RAYS/2; j++) {
          vec4 direction = glm::normalize(vec4(vec3((float) x + apertureSize*i, (float) y + apertureSize*j, camera.focalLength) * camera.R, 1));
          color += Light(camera.position, direction, 0);
          tempDepth += intersection.distance;
        }
      }
      color /= NUM_RAYS * NUM_RAYS;
      tempDepth /= NUM_RAYS * NUM_RAYS;
#endif
      depth[x + SCREEN_WIDTH/2][y + SCREEN_HEIGHT/2] = tempDepth;
      if (tempDepth > maxDepth) {
        maxDepth = tempDepth;
      }
      PutPixelSDL(screen, x + SCREEN_WIDTH/2, y + SCREEN_HEIGHT/2, color, samples);
    }
  }

  // for (int y = 0; y < SCREEN_HEIGHT; y++) {
  //   for (int x = 0; x < SCREEN_WIDTH; x++) {
  //     if (depth[x][y] == 0) PutPixelSDL(screen, x, y, vec3(0));
  //     PutPixelSDL(screen, x, y, vec3(1 - (depth[x][y] / maxDepth)));
  //   }
  // }
}

/*Place updates of parameters here*/
void Update(Camera &camera, screen *screen) {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2 - t);
  t = t2;
  cout << "Render time: " << dt << " ms." << endl;
  /* Update variables*/

  camera.movement = vec3(0);
  vec3 tempRot = camera.rotation;

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_W]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x -= camera.rotationSpeed * dt;
    } else {
      camera.movement.z += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_S]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x += camera.rotationSpeed * dt;
    } else {
      camera.movement.z -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_A]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y += camera.rotationSpeed * dt;
    } else {
      camera.movement.x -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_D]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y -= camera.rotationSpeed * dt;
    } else {
      camera.movement.x += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_Q]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z += camera.rotationSpeed * dt;
    } else {
      camera.movement.y += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_E]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z -= camera.rotationSpeed * dt;
    } else {
      camera.movement.y -= camera.movementSpeed * dt;
    }
  }

  camera.R = CalcRotationMatrix(camera.rotation.x, camera.rotation.y, camera.rotation.z);
  camera.position += vec4(vec3(camera.movement * camera.R), 0);
  if (camera.movement != vec3(0) || camera.rotation != tempRot) {
    memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
    memset(screen->pixels, 0, screen->height * screen->width * sizeof(vec3));
    samples = 0;
  }
}

bool ClosestIntersection(vec4 start, vec4 dir, Intersection &closestIntersection) {
  closestIntersection.distance = m;
  closestIntersection.shapeIndex = -1;

  for (uint index = 0; index < shapes.size(); index++) {
    Shape *shape = shapes[index];
    float dist = shape->intersects(start, dir);
    if (dist > 0 && dist < closestIntersection.distance) {
      closestIntersection.distance = dist;
      closestIntersection.shapeIndex = index;
      closestIntersection.position = start + dist * dir;
    }
  }

  return closestIntersection.shapeIndex != -1;
}

std::default_random_engine generator; 
std::uniform_real_distribution<float> distribution(0, 1);

vec3 IndirectLight(const Intersection &intersection, vec4 dir, int bounce, bool spec) {
#if BOUNCES == 0
    return indirectLighting;
#else
  if (bounce == 0) {
    return vec3(0);
  } else {
    vec4 n = shapes[intersection.shapeIndex]->getNormal(intersection.position);
    vec3 light = vec3(0);
    vec3 Nt, Nb;
    createCoordinateSystem(vec3(n), Nt, Nb);
    int count = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
      if (rand() / RAND_MAX > 0.5) continue;
      else count++;
      Intersection reflectIntersection;
      float r1 = distribution(generator);
      float r2 = distribution(generator);
      vec3 sample = uniformSampleHemisphere(r1, r2); 
      vec3 sampleWorld;
      if (spec) {
        sampleWorld = vec3(mat3(Nb, vec3(glm::reflect(dir, n)), Nt) * sample);
        vec3 P = IndirectLight(reflectIntersection, vec4(sampleWorld, 1), bounce - 1, spec);
        light += (P * max(glm::dot(vec4(sampleWorld, 1), dir), 0.0f));
      } else {
        sampleWorld = vec3(mat3(Nb, vec3(n), Nt) * sample);
        vec4 rayDir = vec4(sampleWorld, 1);
        if (ClosestIntersection(intersection.position + (rayDir * 1e-4f), rayDir, reflectIntersection)) {
          vec3 tint = shapes[reflectIntersection.shapeIndex]->color;
          vec3 P = DirectLight(intersection, dir, false) + IndirectLight(reflectIntersection, rayDir, bounce - 1, spec);
          light += (P * max(glm::dot(rayDir, n), 0.0f)) * tint;
        }
      }
    }
    light /= count;;
    return light;
  }
#endif
}

vec3 DirectLight(const Intersection &intersection, vec4 dir, bool spec) {
  vec3 P = lightColor;
  vec4 n = shapes[intersection.shapeIndex]->getNormal(intersection.position);
  vec4 r = lightPos - intersection.position;
  float rL = glm::length(r);
  vec4 rN = r / rL;
  Intersection shadowIntersection;
  if (ClosestIntersection(intersection.position + (rN * 1e-4f), rN, shadowIntersection)) {
    float distance = glm::distance(intersection.position, shadowIntersection.position);
    if (distance < rL) {
      return vec3(0);
    }
  }
  if (spec) {
    vec4 reflected = glm::reflect(rN, n);
    float shininess = shapes[intersection.shapeIndex]->shininess;
    return (P * max(powf(glm::dot(reflected, dir), shininess), 0.0f)) / (float) (4 * M_PI * powf(rL, 2));
  } else {
    return (P * max(glm::dot(rN, n), 0.0f)) / (float) (4 * M_PI * powf(rL, 2));
  }
}

vec3 Light(const vec4 start, const vec4 dir, int bounce) {
  Intersection intersection;
  if (ClosestIntersection(start, dir, intersection)) {

    Shape *obj = shapes[intersection.shapeIndex];
    // Russian roulette termination
    float U = rand() / (float) RAND_MAX;
    if (bounce > MIN_BOUNCES && (bounce > MAX_BOUNCES || U > max3(obj->color))) {
      // terminate
      return vec3(0);
    }
    
    vec4 hitPos = intersection.position;
    vec4 normal = obj->getNormal(hitPos);

    // Direct Light
    vec3 directDiffuseLight = vec3(0);
    vec3 directSpecularLight = vec3(0);
    for (Shape *light : shapes) {
      if (light->isLight()) {
        vec4 lightPos = light->randomPoint();
        float lightDist = glm::distance(lightPos, hitPos);
        vec4 lightDir = (lightPos - hitPos) / lightDist;
        if (bounce == 0) {
          vec4 reflected = glm::reflect(lightDir, normal);
          directSpecularLight += (light->emit * max(powf(glm::dot(reflected, dir), obj->shininess), 0.0f)) / (float) (4 * M_PI * powf(lightDist, 2));
        }
        Intersection lightIntersection;
        if (ClosestIntersection(hitPos, lightDir, lightIntersection)) {
          if (light == obj) {
            directDiffuseLight += light->emit * max(glm::dot(lightDir, normal), 0.0f) / (float) (4 * M_PI * powf(lightDist, 2));
          }
        }
      }
    }

    // Indirect Light
    vec3 Nt, Nb;
    createCoordinateSystem(vec3(normal), Nt, Nb);
    float r1 = distribution(generator);
    float r2 = distribution(generator);
    vec3 sample = uniformSampleHemisphere(r1, r2); 
    vec3 sampleWorld = vec3(mat3(Nb, vec3(normal), Nt) * sample);
    vec4 rayDir = vec4(sampleWorld, 1);
    vec3 indirectLight = Light(hitPos, rayDir, bounce + 1);


    // if (bounce == 0) {
      return obj->emit + obj->Kd * obj->color * (directDiffuseLight + indirectLight) /*+ obj->Ks * directSpecularLight*/;
    // } else {
      // return obj->color * (directLight + indirectLight);
    // }
  }
  return vec3(0);
}

mat3 CalcRotationMatrix(float x, float y, float z) {
  mat3 Rx = mat3(vec3(1, 0, 0), vec3(0, cosf(x), sinf(x)), vec3(0, -sinf(x), cosf(x)));
  mat3 Ry = mat3(vec3(cosf(y), 0, -sinf(y)), vec3(0, 1, 0), vec3(sinf(y), 0, cosf(y)));
  mat3 Rz = mat3(vec3(cosf(z), sinf(z), 0), vec3(-sinf(z), cosf(z), 0), vec3(0, 0, 1));
  return Rx * Ry * Rz;
}

vec3 uniformSampleHemisphere(const float &r1, const float &r2) {
  float sinTheta = sqrtf(1 - r1 * r1); 
  float phi = 2 * M_PI * r2; 
  float x = sinTheta * cosf(phi); 
  float z = sinTheta * sinf(phi); 
  return vec3(x, r1, z); 
}

void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb) { 
  if (fabs(N.x) > fabs(N.y)) 
      Nt = glm::normalize(vec3(N.z, 0, -N.x)); 
  else 
      Nt = glm::normalize(vec3(0, -N.z, N.y)); 
  Nb = glm::cross(N, Nt); 
}

float max3(vec3 v) {
  return max(v.x, max(v.y, v.z));
}

void LoadModel(vector<Shape *> &scene, const char *path) {
  attrib_t attrib;
  vector<shape_t> shapes;
  vector<material_t> materials;
  std::string error;
  // NB: Lib automatically triangulises -- can be disabled, but is default true
  bool ret = tinyobj::LoadObj(
    &attrib,
    &shapes,
    &materials,
    &error,
    path
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

        real_t tx = attrib.texcoords[2*idx.texcoord_index+0];
        real_t ty = attrib.texcoords[2*idx.texcoord_index+1];

        // Optional: vertex colors
        // tinyobj::real_t red = attrib.colors[3*idx.vertex_index+0];
        // tinyobj::real_t green = attrib.colors[3*idx.vertex_index+1];
        // tinyobj::real_t blue = attrib.colors[3*idx.vertex_index+2];
      }
      index_offset += fv;

      // per-face material
      tinyobj::material_t material = materials[shapes[s].mesh.material_ids[f]];

      scene.push_back(new Triangle(
        verticies[0],
        verticies[1],
        verticies[2],
        vec3(material.emission[0], material.emission[1], material.emission[2]),
        vec3(1, 0, 0),
        material.shininess,
        dot(vec3(1), vec3(material.specular[0], material.specular[1], material.specular[2])),
        dot(vec3(1), vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]))
      ));
    }
  }
}
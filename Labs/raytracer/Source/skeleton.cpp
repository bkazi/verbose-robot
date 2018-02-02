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

using namespace std;
using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false
#define NUM_RAYS 0
#define BOUNCES 1
#define NUM_SAMPLES 32

float m = numeric_limits<float>::max();
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1, 1, 1); 
vec3 indirectLighting = 0.5f * vec3(1, 1, 1);
float shininess = 50;
float specularity = 0.1;

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

struct Intersection {
  vec4 position;
  float distance;
  int triangleIndex;
};

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update(Camera &camera);
void Draw(screen *screen, Camera camera, vector<Triangle> scene);
bool ClosestIntersection(vec4 start, vec4 dir, vector<Triangle> &triangles, Intersection &closestIntersection);
vec3 DirectLight(const Intersection &intersection, vec4 dir, vector<Triangle> &scene, bool spec);
vec3 IndirectLight(const Intersection &intersection, vec4 dir, vector<Triangle> &scene, int bounce, bool spec);
mat3 CalcRotationMatrix(float x, float y, float z);
vec3 uniformSampleHemisphere(const float &r1, const float &r2);
void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb);

int main(int argc, char *argv[]) {

  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  vector<Triangle> scene;
  LoadTestModel(scene);

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
  // while (NoQuitMessageSDL()) {
    Update(camera);
    Draw(screen, camera, scene);
    SDL_Renderframe(screen);
  // }

  SDL_SaveImage(screen, "screenshot.bmp");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen *screen, Camera camera, vector<Triangle> scene) {
  /* Clear buffer */
  memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));

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
      if (NUM_RAYS <= 1) {
        vec4 direction = glm::normalize(vec4(vec3((float) x, (float) y, camera.focalLength) * camera.R, 1));
        if (ClosestIntersection(camera.position, direction, scene, intersection)) {
          color += (DirectLight(intersection, direction, scene, true) + specularity * IndirectLight(intersection, direction, scene, BOUNCES, true) + (1 - specularity) * IndirectLight(intersection, direction, scene, BOUNCES, false)) * scene[intersection.triangleIndex].color;
          tempDepth += intersection.distance;
        }
      } else {
        for (int i = -NUM_RAYS/2; i < NUM_RAYS/2; i++) {
          float ep = (float) 1 / NUM_RAYS;
          vec4 direction = glm::normalize(vec4(vec3((float) x + ep*i, (float) y + ep*i, camera.focalLength) * camera.R, 1));
          if (ClosestIntersection(camera.position, direction, scene, intersection)) {
            color += (DirectLight(intersection, direction, scene, true) + specularity * IndirectLight(intersection, direction, scene, BOUNCES, true) + (1 - specularity) * IndirectLight(intersection, direction, scene, BOUNCES, false)) * scene[intersection.triangleIndex].color;
            tempDepth += intersection.distance;
          }
        }
        color /= NUM_RAYS;
        tempDepth /= NUM_RAYS;
      }
      depth[x + SCREEN_WIDTH/2][y + SCREEN_HEIGHT/2] = tempDepth;
      if (tempDepth > maxDepth) {
        maxDepth = tempDepth;
      }
      PutPixelSDL(screen, x + SCREEN_WIDTH/2, y + SCREEN_HEIGHT/2, color);
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
void Update(Camera &camera) {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2 - t);
  t = t2;
  cout << "Render time: " << dt << " ms.\n";
  /* Update variables*/

  camera.movement = vec3(0);

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_W]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x += camera.rotationSpeed * dt;
    } else {
      camera.movement.z += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_S]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x -= camera.rotationSpeed * dt;
    } else {
      camera.movement.z -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_A]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y -= camera.rotationSpeed * dt;
    } else {
      camera.movement.x -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_D]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y += camera.rotationSpeed * dt;
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
}

bool ClosestIntersection(vec4 start, vec4 dir, vector<Triangle> &triangles, Intersection &closestIntersection) {
  closestIntersection.distance = m;
  closestIntersection.triangleIndex = -1;

  for (uint index = 0; index < triangles.size(); index++) {
    Triangle triangle = triangles[index];
    vec4 v0 = triangle.v0;
    vec4 v1 = triangle.v1;
    vec4 v2 = triangle.v2;
    vec3 e1 = vec3(v1) - vec3(v0);
    vec3 e2 = vec3(v2) - vec3(v0);
    vec3 b = vec3(start) - vec3(v0);
    mat3 A(-vec3(dir), e1, e2);
    float detA = glm::determinant(A);
    float dist = glm::determinant(mat3(b, e1, e2)) / detA;
    
    // Calculate point of intersection
    if (dist > 0 && dist < closestIntersection.distance) {
      float u = glm::determinant(mat3(-vec3(dir), b, e2)) / detA;
      float v = glm::determinant(mat3(-vec3(dir), e1, b)) / detA;
      if (u >= 0 && v >= 0 && u + v <= 1) {
        closestIntersection.distance = dist;
        closestIntersection.triangleIndex = index;
        closestIntersection.position = vec4(vec3(v0) + (mat3(vec3(0), e1, e2) * vec3(0, u, v)), 1);
      }
    }
  }

  return closestIntersection.triangleIndex != -1;
}

std::default_random_engine generator; 
std::uniform_real_distribution<float> distribution(0, 1);

vec3 IndirectLight(const Intersection &intersection, vec4 dir, vector<Triangle> &scene, int bounce, bool spec) {
  if (BOUNCES == 0) {
    return indirectLighting;
  }
  if (bounce == 0) {
    return vec3(0);
  } else {
    vec4 n = scene[intersection.triangleIndex].normal;
    if (spec) {
      Intersection reflectIntersection;
      vec4 reflect = glm::normalize(glm::reflect(dir, n));
      if (ClosestIntersection(intersection.position + (reflect * 1e-4f), reflect, scene, reflectIntersection)) {
        vec3 P = (DirectLight(reflectIntersection, reflect, scene, false) + IndirectLight(reflectIntersection, reflect, scene, bounce - 1, true)) * scene[reflectIntersection.triangleIndex].color;
        return (P * max(glm::dot(reflect, n), 0.0f));
      }
    } else {
      vec3 light = vec3(0);
      vec3 Nt, Nb;
      createCoordinateSystem(vec3(n), Nt, Nb);
      for (int i = 0; i < NUM_SAMPLES; i++) {
        Intersection reflectIntersection;
        float r1 = distribution(generator);
        float r2 = distribution(generator);
        vec3 sample = uniformSampleHemisphere(r1, r2); 
        vec3 sampleWorld(mat3(Nb, vec3(n), Nt) * sample);
        vec4 rayDir = vec4(sampleWorld, 1);
        if (ClosestIntersection(intersection.position + (rayDir * 1e-4f), rayDir, scene, reflectIntersection)) {
          vec3 P = (DirectLight(reflectIntersection, rayDir, scene, false) + IndirectLight(reflectIntersection, rayDir, scene, bounce - 1, false)) * scene[reflectIntersection.triangleIndex].color;
          light += (P * max(glm::dot(rayDir, n), 0.0f));
        }
      }
      light /= NUM_SAMPLES;
      return light;
    }
    return vec3(0);
  }
}

vec3 DirectLight(const Intersection &intersection, vec4 dir, vector<Triangle> &scene, bool spec) {
  vec3 P = lightColor;
  vec4 n = scene[intersection.triangleIndex].normal;
  vec4 r = lightPos - intersection.position;
  float rL = glm::length(r);
  vec4 rN = r / rL;
  Intersection shadowIntersection;
  if (ClosestIntersection(intersection.position + (n * 1e-4f), rN, scene, shadowIntersection)) {
    float distance = glm::distance(intersection.position, shadowIntersection.position);
    if (distance < rL) {
      return vec3(0);
    }
  }
  vec4 reflected = glm::reflect(dir, n);
  if (spec) {
    return (P * (max(glm::dot(rN, n), 0.0f) + max(powf(glm::dot(reflected, rN), shininess), 0.0f))) / (float) (4 * M_PI * powf(rL, 2));
  } else {
    return (P * max(glm::dot(rN, n), 0.0f)) / (float) (4 * M_PI * powf(rL, 2));
  }
}

mat3 CalcRotationMatrix(float x, float y, float z) {
  mat3 Rx = mat3(vec3(1, 0, 0), vec3(0, cosf(x), sinf(x)), vec3(0, -sinf(x), cosf(x)));
  mat3 Ry = mat3(vec3(cosf(y), 0, -sinf(y)), vec3(0, 1, 0), vec3(sinf(y), 0, cosf(y)));
  mat3 Rz = mat3(vec3(cosf(z), sinf(z), 0), vec3(-sinf(z), cosf(z), 0), vec3(0, 0, 1));
  return Rx * Ry * Rz;
}

vec3 uniformSampleHemisphere(const float &r1, const float &r2) { 
  // cos(theta) = u1 = y
  // cos^2(theta) + sin^2(theta) = 1 -> sin(theta) = srtf(1 - cos^2(theta))
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
#include <iostream>
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
#define NUM_RAYS 2
#define BOUNCES 2

float m = numeric_limits<float>::max();
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1, 1, 1); 
vec3 indirectLighting = 0.5f * vec3(1, 1, 1);

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
vec3 DirectLight(const Intersection &intersection, vector<Triangle> &scene);
vec3 IndirectLight(const Intersection &intersection, vec4 dir, vector<Triangle> &scene, int bounce);

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

  while (NoQuitMessageSDL()) {
    Update(camera);
    Draw(screen, camera, scene);
    SDL_Renderframe(screen);
  }

  SDL_SaveImage(screen, "screenshot.bmp");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen *screen, Camera camera, vector<Triangle> scene) {
  /* Clear buffer */
  memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));

  Intersection intersection;
  for (int y = -SCREEN_HEIGHT/2; y < SCREEN_HEIGHT/2; y++) {
    for (int x = -SCREEN_WIDTH/2; x < SCREEN_WIDTH/2; x++) {
      vec3 color = vec3(0);
      for (int i = -NUM_RAYS/2; i < NUM_RAYS/2; i++) {
        for (int j = -NUM_RAYS/2; j < NUM_RAYS/2; j++) {
          float ep = (float) 1 / NUM_RAYS;
          vec4 direction = glm::normalize(vec4(vec3((float) x + ep*j, (float) y + ep*i, camera.focalLength) * camera.R, 1));
          if (ClosestIntersection(camera.position, direction, scene, intersection)) {
            color += (DirectLight(intersection, scene) + IndirectLight(intersection, direction, scene, BOUNCES)) * scene[intersection.triangleIndex].color;
          }
        }
      }
      color /= NUM_RAYS * NUM_RAYS;
      PutPixelSDL(screen, x + SCREEN_WIDTH/2, y + SCREEN_HEIGHT/2, color);
    }
  }
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

  mat3 Rx = mat3(vec3(1, 0, 0), vec3(0, cos(camera.rotation.x), sin(camera.rotation.x)), vec3(0, -sin(camera.rotation.x), cos(camera.rotation.x)));
  mat3 Ry = mat3(vec3(cos(camera.rotation.y), 0, -sin(camera.rotation.y)), vec3(0, 1, 0), vec3(sin(camera.rotation.y), 0, cos(camera.rotation.y)));
  mat3 Rz = mat3(vec3(cos(camera.rotation.z), sin(camera.rotation.z), 0), vec3(-sin(camera.rotation.z), cos(camera.rotation.z), 0), vec3(0, 0, 1));
  camera.R = Rx * Ry * Rz;
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

vec3 IndirectLight(const Intersection &intersection, vec4 dir, vector<Triangle> &scene, int bounce) {
  if (BOUNCES == 0) {
    return indirectLighting;
  }
  if (bounce == 0) {
    return vec3(0);
  } else {
    vec4 n = scene[intersection.triangleIndex].normal;
    vec4 reflect = glm::normalize(glm::reflect(dir, n));
    Intersection reflectIntersection;
    if (ClosestIntersection(intersection.position, reflect, scene, reflectIntersection)) {
      float rL = glm::distance(intersection.position, reflectIntersection.position);
      vec3 P = (DirectLight(reflectIntersection, scene) + IndirectLight(reflectIntersection, reflect, scene, bounce - 1)) * scene[reflectIntersection.triangleIndex].color;
      return (P * max(glm::dot(reflect, n), 0.0f)) / (float) (4 * M_PI * pow(rL, 2));
    } else {
      return vec3(0);
    }
  }
}

vec3 DirectLight(const Intersection &intersection, vector<Triangle> &scene) {
  vec3 P = lightColor;
  vec4 n = scene[intersection.triangleIndex].normal;
  vec4 r = lightPos - intersection.position;
  vec4 rN = glm::normalize(r);
  float rL = glm::length(r);
  Intersection shadowIntersection;
  if (ClosestIntersection(intersection.position, rN, scene, shadowIntersection)) {
    float distance = glm::distance(intersection.position, shadowIntersection.position);
    if (distance < rL) {
      return vec3(0);
    }
  }
  return (P * max(glm::dot(rN, n), 0.0f)) / (float) (4 * M_PI * pow(rL, 2));
}
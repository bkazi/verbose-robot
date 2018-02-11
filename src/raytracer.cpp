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
#include "TestModel.h"
#include "objects.h"

using namespace std;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480
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
  LoadModel(shapes, path);

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

  int x, y;
  #pragma omp parallel for private(x, y)
  for (y = -SCREEN_HEIGHT/2; y < SCREEN_HEIGHT/2; y++) {
    for (x = -SCREEN_WIDTH/2; x < SCREEN_WIDTH/2; x++) {
      vec3 color = vec3(0);
#if NUM_RAYS <= 1
      vec4 direction = glm::normalize(vec4(vec3(x, y, camera.focalLength) * camera.R, 1));
      color += Light(camera.position, direction, 0);
#else
      for (int i = -NUM_RAYS/2; i < NUM_RAYS/2; i++) {
        for (int j = -NUM_RAYS/2; j < NUM_RAYS/2; j++) {
          vec4 direction = glm::normalize(vec4(vec3((float) x + apertureSize*i, (float) y + apertureSize*j, camera.focalLength) * camera.R, 1));
          color += Light(camera.position, direction, 0);
        }
      }
      color /= NUM_RAYS * NUM_RAYS;
#endif
      PutPixelSDL(screen, x + SCREEN_WIDTH/2, y + SCREEN_HEIGHT/2, color, samples);
    }
  }
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

vec3 Light(const vec4 start, const vec4 dir, int bounce) {
  Intersection intersection;
  if (ClosestIntersection(start + start * 1e-4f, dir, intersection)) {

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
        vec4 lightVec = lightPos - hitPos;
        float lightDist = glm::length(lightVec);
        vec4 lightDir = lightVec / lightDist;
        Intersection lightIntersection;
        if (ClosestIntersection(hitPos + lightDir * 1e-4f, lightDir, lightIntersection)) {
          if (light == shapes[lightIntersection.shapeIndex]) {
            vec4 reflected = glm::reflect(lightDir, normal);
            directSpecularLight += light->emit * max(powf(glm::dot(reflected, dir), obj->shininess), 0.0f) / (float) (4 * M_PI * powf(lightDist, 2));
            directDiffuseLight += light->emit * max(glm::dot(lightDir, normal), 0.0f) / (float) (4 * M_PI * powf(lightDist, 2));
          }
        }
      }
    }

    // Indirect Light
    vec3 indirectLight = vec3(0);
    vec3 Nt, Nb;
    createCoordinateSystem(vec3(normal), Nt, Nb);
    float r1 = distribution(generator);
    float r2 = distribution(generator);
    vec3 sample = uniformSampleHemisphere(r1, r2); 
    vec3 sampleWorld = vec3(mat3(Nb, vec3(normal), Nt) * sample);
    vec4 rayDir = vec4(sampleWorld, 1);
    indirectLight += obj->Kd * Light(hitPos, rayDir, bounce + 1);

    if (bounce == 0) {
      return obj->emit + obj->color * (obj->Kd * directDiffuseLight + obj->Ka * indirectLight + obj->Ks * directSpecularLight);
    } else {
      return obj->color * (obj->Kd * directDiffuseLight + obj->Ka * indirectLight);
    }
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
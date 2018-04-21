#include <iostream>
#include <cstdlib>
#include <random> 
#include <ctime>
#include <cstdint>
#include <climits>
#include <cmath>
#include <SDL.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <random>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include "scene.h"
#include "objects.h"
#include "camera.h"

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
/* FUNCTIONS                                                                   */

void Update();
void Draw(screen *screen);
bool ClosestIntersection(vec4 start, vec4 dir, Intersection &closestIntersection);
vec3 DirectLight(const Intersection &intersection, vec4 dir, bool spec);
vec3 IndirectLight(const Intersection &intersection, vec4 dir, int bounce,
                   bool spec);
mat3 CalcRotationMatrix(float x, float y, float z);
vec3 uniformSampleHemisphere(const float &r1, const float &r2);
void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb);
vec3 Light(const vec4 start, const vec4 dir, int bounce = 0);
float max3(vec3);
void LoadModel(vector<Object *> &scene, const char *path);

float samples = 0;
Scene *scene;
Camera *camera;

int main(int argc, char *argv[]) {
  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  scene = new Scene();
  scene->LoadTest();
  if (argc >= 2) {
    const string path = argv[1];
    scene->LoadModel(path);
  }

  // scene->createBVH();

  camera = new Camera(
    vec4(0, 0, -3.001, 1),
    vec3(0, 0, 0),
    SCREEN_HEIGHT,
    0.001,
    0.001
  );
  
  srand(42);
  memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
#ifdef LIVE
  while (NoQuitMessageSDL()) {
    Update();
    Draw(screen);
    SDL_Renderframe(screen);
  }
#else
  Update();
  Draw(screen);
  SDL_Renderframe(screen);
  Update();
#endif

  SDL_SaveImage(screen, "screenshot.bmp");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen *screen) {
  samples++;

  int x, y;
#pragma omp parallel for private(x, y)
  for (y = -SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT / 2; y++) {
    for (x = -SCREEN_WIDTH / 2; x < SCREEN_WIDTH / 2; x++) {
      vec3 color = vec3(0);
#if NUM_RAYS <= 1
      vec4 direction = glm::normalize(vec4(x, y, camera->focalLength, 1) * camera->getRotationMatrix());
      color += Light(camera->position, direction);
#else
      for (int i = -NUM_RAYS/2; i < NUM_RAYS/2; i++) {
        for (int j = -NUM_RAYS/2; j < NUM_RAYS/2; j++) {
          vec4 direction = glm::normalize(vec4(vec3((float) x + apertureSize*i, (float) y + apertureSize*j, camera->focalLength) * camera->getRotationMatrix(), 1));
          color += Light(camera->position, direction);
        }
      }
      color /= NUM_RAYS * NUM_RAYS;
#endif
      PutPixelSDL(screen, x + SCREEN_WIDTH / 2, y + SCREEN_HEIGHT / 2, color,
                  samples);
    }
  }
}

/*Place updates of parameters here*/
void Update() {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2 - t);
  t = t2;
  cout << "Render time: " << dt << " ms." << endl;
  /* Update variables*/

  camera->update(dt);
}

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0, 1);

vec3 Light(const vec4 start, const vec4 dir, int bounce) {
  Intersection intersection;
  if (scene->intersect(new Ray(start + dir * 1e-4f, dir), intersection)) {
    // Russian roulette termination
    float U = rand() / (float)RAND_MAX;
    if (bounce > MIN_BOUNCES &&
        (bounce > MAX_BOUNCES || U > max3(intersection.primitive->color))) {
      // terminate
      return vec3(0);
    }

    vec4 hitPos = intersection.position;
    vec4 normal = intersection.primitive->getNormal(hitPos);

    // Direct Light
    vec3 directDiffuseLight = vec3(0);
    vec3 directSpecularLight = vec3(0);
    for (Object *object : scene->objects) {
      for (Primitive *light : object->primitives) {
        if (light->isLight()) {
          vec4 lightPos = light->randomPoint();
          vec4 lightVec = lightPos - hitPos;
          float lightDist = glm::length(lightVec);
          vec4 lightDir = lightVec / lightDist;
          Intersection lightIntersection;
          if (scene->intersect(new Ray(hitPos + lightDir * 1e-4f, lightDir),
                                  lightIntersection)) {
            if (light == lightIntersection.primitive) {
              vec4 reflected = glm::reflect(lightDir, normal);
              directSpecularLight +=
                  light->emit *
                  max(powf(glm::dot(reflected, dir), intersection.primitive->shininess),
                      0.0f) /
                  (float)(4 * M_PI * powf(lightDist, 2));
              directDiffuseLight += light->emit *
                                    max(glm::dot(lightDir, normal), 0.0f) /
                                    (float)(4 * M_PI * powf(lightDist, 2));
            }
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
    indirectLight += intersection.primitive->Kd * Light(hitPos, rayDir, bounce + 1);

    if (bounce == 0) {
      return intersection.primitive->emit +
             intersection.primitive->color * (intersection.primitive->Kd * directDiffuseLight +
                                 intersection.primitive->Ka * indirectLight +
                                 intersection.primitive->Ks * directSpecularLight);
    } else {
      return intersection.primitive->color * (intersection.primitive->Kd * directDiffuseLight +
                                 intersection.primitive->Ka * indirectLight);
    }
  }
  return vec3(0);
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

float max3(vec3 v) { return max(v.x, max(v.y, v.z)); }
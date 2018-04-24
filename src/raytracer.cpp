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
#include "raytracer_screen.h"
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
using glm::ivec2;

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480
#define FULLSCREEN_MODE false
#define MIN_BOUNCES 10
#define MAX_BOUNCES 20
// #define AA
#define LIVE

float m = numeric_limits<float>::max();
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1, 1, 1);
vec3 indirectLighting = 0.5f * vec3(1, 1, 1);
float apertureSize = 0.1;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update(screen *screen);
void Draw(screen *screen);
bool ClosestIntersection(vec4 start, vec4 dir, Intersection &closestIntersection);
vec3 DirectLight(const Intersection &intersection, vec4 dir, bool spec);
vec3 IndirectLight(const Intersection &intersection, vec4 dir, int bounce,
                   bool spec);
mat3 CalcRotationMatrix(float x, float y, float z);
vec3 uniformSampleHemisphere(const float &r1, const float &r2);
vec3 sampleConeBase(float b);
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
  if (argc >= 2) {
    const string path = argv[1];
    scene->LoadModel(path);
  } else {
    scene->LoadTest();
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
    Update(screen);
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
#pragma omp parallel for private(x, y) collapse(2)
  for (y = -SCREEN_HEIGHT / 2; y < SCREEN_HEIGHT / 2; y++) {
    for (x = -SCREEN_WIDTH / 2; x < SCREEN_WIDTH / 2; x++) {
      vec3 color = vec3(0);
#ifndef AA
      vec4 direction = glm::normalize(vec4(x, y, camera->focalLength, 1) * camera->getRotationMatrix());
      color += Light(camera->position, direction);
#else
      ivec2 samplePoints[5] = {
        ivec2(x - apertureSize, y - apertureSize),
        ivec2(x + apertureSize, y - apertureSize),
        ivec2(x + apertureSize, y + apertureSize),
        ivec2(x - apertureSize, y + apertureSize),
        ivec2(x, y),
      };
      for (int i = 0; i < 5; i++) {
        vec4 direction = glm::normalize(vec4((float) samplePoints[i].x, (float) samplePoints[i].y, camera->focalLength, 1) * camera->getRotationMatrix());
        color += Light(camera->position, direction);
      }
      color /= 5.f;
#endif
      PutPixelSDL(screen, x + SCREEN_WIDTH / 2, y + SCREEN_HEIGHT / 2, color,
                  samples);
    }
  }
}

/*Place updates of parameters here*/
void Update(screen *screen) {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2 - t);
  t = t2;
  cout << "Render time: " << dt << " ms." << endl;
  /* Update variables*/

  if (camera->update(dt)) {
    memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
    memset(screen->pixels, 0, screen->height * screen->width * sizeof(vec3));
    samples = 0;
  }
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
                      0.0f) * max(glm::dot(lightDir, normal), 0.0f) /
                  (float)(4 * M_PI * powf(lightDist, 2));
              directDiffuseLight += light->emit *
                                    max(glm::dot(lightDir, normal), 0.0f) /
                                    (float)(4 * M_PI * powf(lightDist, 2));
            }
          }
        }
      }
    }
    directSpecularLight = glm::clamp(directSpecularLight, vec3(0), vec3(1));
    directDiffuseLight = glm::clamp(directDiffuseLight, vec3(0), vec3(1));

    // Indirect Light
    vec3 indirectLight = vec3(0);
    float prob = intersection.primitive->Kd / (intersection.primitive->Kd + intersection.primitive->Ks);
    if ((rand() / (float) RAND_MAX) < prob) {
      // diffuse
      vec3 Nt, Nb;
      float r1 = distribution(generator);
      float r2 = distribution(generator);
      vec3 sample = uniformSampleHemisphere(r1, r2);
      createCoordinateSystem(vec3(normal), Nt, Nb);
      vec3 sampleWorld = vec3(mat3(Nb, vec3(normal), Nt) * sample);
      vec4 rayDir = vec4(sampleWorld, 1);
      indirectLight += Light(hitPos, rayDir, bounce + 1);
    } else {
      //specular
      vec3 Nt, Nb;
      vec4 reflected = glm::reflect(dir, normal);
      createCoordinateSystem(vec3(reflected), Nt, Nb);
      vec3 sample = sampleConeBase(10.f / intersection.primitive->shininess);
      vec3 sampleWorld = vec3(mat3(Nb, vec3(reflected), Nt) * sample);
      vec4 rayDir = glm::normalize(vec4(sampleWorld, 1));
      indirectLight += Light(hitPos, rayDir, bounce + 1);
    }
    indirectLight = glm::clamp(
      indirectLight, vec3(0), vec3(1)
    );

    return intersection.primitive->emit +
            intersection.primitive->color * (intersection.primitive->Kd * directDiffuseLight +
                                intersection.primitive->Ka * indirectLight +
                                intersection.primitive->Ks * directSpecularLight);
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

vec3 sampleConeBase(float b) {
  float r = b * sqrt(rand() / (float) RAND_MAX);
  float t = 2 * M_PI * (rand() / (float) RAND_MAX);
  return vec3(r * cos(t), 1, r * sin(t));
}

float max3(vec3 v) { return max(v.x, max(v.y, v.z)); }
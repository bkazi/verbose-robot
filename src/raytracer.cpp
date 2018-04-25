#include <SDL.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <random>
#include "TestModel.h"
#include "camera.h"
#include "objects.h"
#include "raytracer_screen.h"
#include "scene.h"

using namespace std;
using glm::ivec2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#define SCREEN_WIDTH 4096
#define SCREEN_HEIGHT 2160
#define FULLSCREEN_MODE false
#define MIN_BOUNCES 20
#define MAX_BOUNCES 30
#define AA
#define LIVE

float m = numeric_limits<float>::max();
vec4 lightPos(0, -0.5, -0.7, 1.0);
vec3 lightColor = 14.f * vec3(1, 1, 1);
vec3 indirectLighting = 0.5f * vec3(1, 1, 1);
float apertureSize = 0.2f;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS */

void Update(screen *screen);
void Draw(screen *screen);
bool ClosestIntersection(vec4 start, vec4 dir,
                         Intersection &closestIntersection);
mat3 CalcRotationMatrix(float x, float y, float z);
vec3 uniformSampleHemisphere(const float &r1, const float &r2);
vec3 sampleConeBase(float b);
void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb);
vec3 Light(const vec4 start, const vec4 dir, float currIor = 1.f,
           int bounce = 0);
void fresnel(vec4 I, vec4 N, float ior, float &kr);
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

  camera = new Camera(vec4(0, 0, -3.001, 1), vec3(0, 0, 0), SCREEN_HEIGHT,
                      0.001, 0.001);

  srand(42);
  memset(screen->buffer, 0, screen->height * screen->width * sizeof(uint32_t));
#ifdef LIVE
  while (NoQuitMessageSDL()) {
    Update(screen);
    Draw(screen);
    SDL_Renderframe(screen);
    SDL_SaveImage(screen, "screenshot.bmp");
  }
#else
  Update();
  Draw(screen);
  SDL_Renderframe(screen);
  Update();
#endif

  SDL_SaveImage(screen, "screenshot.png");

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
      vec4 direction = glm::normalize(vec4(x, y, camera->focalLength, 1) *
                                      camera->getRotationMatrix());
      color += Light(camera->position, direction);
#else
      ivec2 samplePoints[4] = {
          ivec2(x - apertureSize, y - apertureSize),
          ivec2(x + apertureSize, y - apertureSize),
          ivec2(x + apertureSize, y + apertureSize),
          ivec2(x - apertureSize, y + apertureSize),
      };
      for (int i = 0; i < 4; i++) {
        vec4 direction = glm::normalize(vec4((float)samplePoints[i].x,
                                             (float)samplePoints[i].y,
                                             camera->focalLength, 1) *
                                        camera->getRotationMatrix());
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
    memset(screen->buffer, 0,
           screen->height * screen->width * sizeof(uint32_t));
    memset(screen->pixels, 0, screen->height * screen->width * sizeof(vec3));
    samples = 0;
  }
}

std::default_random_engine generator;
std::uniform_real_distribution<float> distribution(0, 1);

vec3 Light(const vec4 start, const vec4 dir, float currIor, int bounce) {
  Intersection intersection;
  Ray ray;

  ray.position = start + dir * 1e-4f;
  ray.direction = dir;
  if (scene->intersect(ray, intersection)) {
    // Russian roulette termination
    float U = rand() / (float)RAND_MAX;
    if (intersection.primitive->isLight()) {
      return intersection.primitive->material.emission;
    }
    if ((bounce > MIN_BOUNCES &&
        (bounce > MAX_BOUNCES ||
         U > max3(intersection.primitive->material.color)))) {
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

          ray.position = hitPos + lightDir * 1e-4f;
          ray.direction = lightDir;
          if (scene->intersect(ray,
                               lightIntersection)) {
            if (light == lightIntersection.primitive) {
              vec4 reflected = glm::reflect(lightDir, normal);
              directSpecularLight +=
                  light->material.emission *
                  max(powf(glm::dot(reflected, dir),
                           intersection.primitive->material.shininess),
                      0.0f) *
                  max(glm::dot(lightDir, normal), 0.0f) /
                  (float)(4 * M_PI * powf(lightDist, 2));
              directDiffuseLight += light->material.emission *
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
    float prob = dot(intersection.primitive->material.diffuse /
                         (intersection.primitive->material.diffuse +
                          intersection.primitive->material.diffuse),
                     vec3(1.f / 3.f));
    if (intersection.primitive->material.transmittance.x > 0 ||
        intersection.primitive->material.transmittance.y > 0 ||
        intersection.primitive->material.transmittance.z > 0) {
      vec3 refractionColor;
      float kr;
      fresnel(dir, normal, intersection.primitive->material.refractiveIndex,
              kr);
      bool isInside = glm::dot(dir, normal) > 0;
      vec4 bias = 1e-4f * normal;
      float eta = !isInside
                      ? 1.f / intersection.primitive->material.refractiveIndex
                      : intersection.primitive->material.refractiveIndex;
      float newIor =
          isInside ? 1.f : intersection.primitive->material.refractiveIndex;
      normal = isInside ? -normal : normal;
      if (kr < 1) {
        vec4 refracted = glm::normalize(glm::refract(dir, normal, eta));
        vec4 start = isInside ? hitPos + bias : hitPos - bias;
        refractionColor = Light(start, refracted, newIor, bounce + 1);
      }
      vec4 reflected = glm::normalize(glm::reflect(dir, normal));
      vec4 start = isInside ? hitPos + bias : hitPos - bias;
      vec3 reflectionColor = Light(start, reflected, newIor, bounce + 1);
      indirectLight += kr * reflectionColor + (1 - kr) * refractionColor;
    } else if ((rand() / (float)RAND_MAX) < prob) {
      // diffuse
      vec3 Nt, Nb;
      float r1 = distribution(generator);
      float r2 = distribution(generator);
      vec3 sample = uniformSampleHemisphere(r1, r2);
      createCoordinateSystem(vec3(normal), Nt, Nb);
      vec3 sampleWorld = vec3(mat3(Nb, vec3(normal), Nt) * sample);
      vec4 rayDir = vec4(sampleWorld, 1);
      indirectLight +=
          Light(hitPos, rayDir,
                intersection.primitive->material.refractiveIndex, bounce + 1);
    } else {
      // specular
      vec3 Nt, Nb;
      vec4 reflected = glm::reflect(dir, normal);
      createCoordinateSystem(vec3(reflected), Nt, Nb);
      vec3 sample =
          sampleConeBase(10.f / intersection.primitive->material.shininess);
      vec3 sampleWorld = vec3(mat3(Nb, vec3(reflected), Nt) * sample);
      vec4 rayDir = glm::normalize(vec4(sampleWorld, 1));
      indirectLight +=
          Light(hitPos, rayDir,
                intersection.primitive->material.refractiveIndex, bounce + 1);
    }
    indirectLight = glm::clamp(indirectLight, vec3(0), vec3(10));

    return intersection.primitive->material.color *
               (intersection.primitive->material.diffuse * directDiffuseLight +
                intersection.primitive->material.ambient * indirectLight +
                intersection.primitive->material.specular *
                    directSpecularLight);
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
  float r = b * sqrt(rand() / (float)RAND_MAX);
  float t = 2 * M_PI * (rand() / (float)RAND_MAX);
  return vec3(r * cos(t), 1, r * sin(t));
}

void fresnel(vec4 I, vec4 N, float ior, float &kr) {
  float cosi = glm::clamp(glm::dot(I, N), -1.f, 1.f);
  float etai = 1, etat = ior;
  if (cosi > 0) {
    swap(etai, etat);
  }
  // Compute sini using Snell's law
  float sint = etai / etat * sqrtf(max(0.f, 1 - powf(cosi, 2.f)));
  // Total internal reflection
  if (sint >= 1) {
    kr = 1;
  } else {
    float cost = sqrtf(max(0.f, 1 - powf(sint, 2.f)));
    cosi = fabsf(cosi);
    float Rs =
        ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
    float Rp =
        ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
    kr = (Rs * Rs + Rp * Rp) / 2;
  }
  // As a consequence of the conservation of energy, transmittance is given by:
  // kt = 1 - kr;
}

float max3(vec3 v) { return max(v.x, max(v.y, v.z)); }
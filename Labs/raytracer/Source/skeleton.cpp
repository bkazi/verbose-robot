#include <iostream>
#include <glm/glm.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include <stdint.h>
#include <limits.h>

using namespace std;
using glm::mat3;
using glm::mat4;
using glm::vec3;
using glm::vec4;

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false

/* ----------------------------------------------------------------------------*/
/* STRUCTS                                                                     */
struct Camera {
  float focalLength;
  vec4 position;
  vec4 rotation;
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

int main(int argc, char *argv[]) {

  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  vector<Triangle> scene;
  LoadTestModel(scene);

  Camera camera = {
    SCREEN_HEIGHT,
    vec4(0, 0, -3, 1),
    vec4(0, 0, 0, 1),
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

  for (int y = -SCREEN_HEIGHT/2; y < SCREEN_HEIGHT/2; y++) {
    for (int x = -SCREEN_WIDTH/2; x < SCREEN_WIDTH/2; x++) {
      vec4 direction = glm::normalize(vec4(x, y, camera.focalLength, 1));
      Intersection intersection;
      if (ClosestIntersection(camera.position, direction, scene, intersection)) {
        PutPixelSDL(screen, x + SCREEN_WIDTH/2, y + SCREEN_HEIGHT/2, scene[intersection.triangleIndex].color);
      }
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

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_W]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x += camera.rotationSpeed *dt;
    } else {
      camera.position.z += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_S]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x -= camera.rotationSpeed *dt;
    } else {
      camera.position.z -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_A]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y -= camera.rotationSpeed *dt;
    } else {
      camera.position.x -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_D]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y += camera.rotationSpeed *dt;
    } else {
      camera.position.x += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_Q]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z += camera.rotationSpeed *dt;
    } else {
      camera.position.y += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_E]) {if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z -= camera.rotationSpeed *dt;
    } else {
      camera.position.y -= camera.movementSpeed * dt;
    }
  }
}

bool ClosestIntersection(vec4 start, vec4 dir, vector<Triangle> &triangles, Intersection &closestIntersection) {
  vec3 best = vec3(numeric_limits<float>::max(), 0, 0);
  int triangleIndex = -1;
  vec3 position;

  for (uint index = 0; index < triangles.size(); index++) {
    Triangle triangle = triangles[index];
    vec4 v0 = triangle.v0;
    vec4 v1 = triangle.v1;
    vec4 v2 = triangle.v2;
    vec3 e1 = vec3(v1.x - v0.x, v1.y - v0.y, v1.z - v0.z); //v1 - v0;
    vec3 e2 = vec3(v2.x - v0.x, v2.y - v0.y, v2.z - v0.z);
    vec3 b = vec3(start.x - v0.x, start.y - v0.y, start.z - v0.z);
    vec3 d = vec3(dir.x, dir.y, dir.z);
    mat3 A(-d, e1, e2);
    vec3 x = glm::inverse(A) * b;
    
    // Calculate point of intersection
    if (x.y >= 0 && x.z >= 0 && x.y + x.z <= 1) {
      if (x.x >= 0 && x.x < best.x) {
        best = x;
        triangleIndex = index;
        position = vec3(v0.x, v0.y, v0.z) + (x.y * e1) + (x.z * e2);
      }
    }
  }

  closestIntersection.distance = best.x;
  closestIntersection.triangleIndex = triangleIndex;
  closestIntersection.position = vec4(position, 1);

  return triangleIndex != -1;
}
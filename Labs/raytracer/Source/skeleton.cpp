#include <iostream>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
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
void LoadModel(string path);

int main(int argc, char *argv[]) {

  LoadModel("/home/gregory/Dropbox/gtr8/r8_gt_obj.obj");

  // screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  // vector<Triangle> scene;
  // LoadTestModel(scene);

  // Camera camera = {
  //   SCREEN_HEIGHT,
  //   vec4(0, 0, -3, 1),
  //   mat3(1),
  //   vec3(0, 0, 0),
  //   vec3(0, 0, 0),
  //   0.001,
  //   0.001,
  // };

  // while (NoQuitMessageSDL()) {
  //   Update(camera);
  //   Draw(screen, camera, scene);
  //   SDL_Renderframe(screen);
  // }

  // SDL_SaveImage(screen, "screenshot.bmp");

  // KillSDL(screen);
  // return 0;
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
            color += (DirectLight(intersection, scene) + indirectLighting) * scene[intersection.triangleIndex].color;
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

vec3 DirectLight(const Intersection &intersection, vector<Triangle> &scene) {
  vec3 P = lightColor;
  vec4 n = scene[intersection.triangleIndex].normal;
  vec4 r = lightPos - intersection.position;
  vec4 rN = glm::normalize(r);
  float rL = glm::length(r);
  Intersection shadowIntersection;
  if (ClosestIntersection(intersection.position, rN, scene, shadowIntersection)) {
    if (shadowIntersection.distance < rL) {
      return vec3(0);
    }
  }
  return (P * max(glm::dot(rN, n), 0.0f)) / (float) (4 * M_PI * pow(rL, 2));
}

vector<vec3> parseFace(string f1, string f2, string f3, string f4, vector<vec3> vertices, vector<vec2> texture, vector<vec3> normals) {
  if (f4 != "") {
    // Do split and recurse
  }
}

void LoadModel(string path) {
  // See here: https://www.cs.cmu.edu/~mbz/personal/graphics/obj.html
  vector<vec3> vertices;
  vector<vec2> texture;
  vector<vec3> normals;

  vector<vec3> face_vertices;
  vector<vec3> face_texture;
  vector<vec3> face_normals;

  ifstream objFile (path);
  if (!objFile) {
    cerr << "Cannot open " << filename << std::endl;
    exit(1);
  }

  string line;
  while (getline(objFile, line)) {
    if (line.substr(0, 2) == "v ") {
      istringstream v(line.substr(2));
      vec3 vert;
      double x, y, z;
      v >> x >> y >> z;
      vert = vec3(x,y,z);
      vertices.push_back(vert);
    } else if (line.substr(0, 2) == "vt") {
      istringstream vt(line.substr(3));
      vec2 tex;
      int u, v;
      vt >> u >> v;
      tex = vec2(u, v);
      texture.push_back(tex);
    } else if (line.substr(0, 2) == "vn") {
      istringstream vn(line.substr(3));
      vec3 norm;
      int x, y, z;
      vt >> x >> y >> z;
      norm = vec4(x, y, z);
      normals.push_back(norm);
    } else if(line.substr(0, 2) == "f "){
      istringstream f(line.substr(3));
      string f1, f2, f3, f4;

      f >> f1 >> f2 >> f3 >> f4;

      vec3 face = parseFace(f1, f2, f3, f4, vertices, texture, normals);
      
      // Don't look at this, it's awful
      if (f4 != "") {

      } else {
        int f1v, f1t, f1n, f2v, f2t, f2n, f3v, f3t, f3n;
        const char* f1c = f1.c_str(), f2c = f2.c_str(), f3c = f3.c_str();
        sscanf(f1c, "%i/%i/%i", &f1v, &f1t, &f1n);
        sscanf(f2c, "%i/%i/%i", &f2v, &f2t, &f2n);
        sscanf(f3c, "%i/%i/%i", &f3v, &f3t, &f3n);

        if (f1v < 0) {
          f1v += vertices.size();
        }
        if (f2v < 0) {
          f1v += vertices.size();
        }
        if (f3v < 0) {
          f1v += vertices.size();
        }
        if (f1t < 0) {
          f1v += texture.size();
        }
        if (f2t < 0) {
          f1v += texture.size();
        }
        if (f3t < 0) {
          f1v += texture.size();
        }
        if (f1n < 0) {
          f1v += normals.size();
        }
        if (f2n < 0) {
          f1v += normals.size();
        }
        if (f3t < 0) {
          f1n += normals.size();
        }

        face_vertices.push_back(vec3(f1v, f2v, f3v));
        face_texture.push_back(vec3(f1t, f2t, f3t));
        face_normals.push_back(vec3(f1n, f2n, f3n));
      }
    }
  }
}
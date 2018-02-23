#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModelH.h"
#include <stdint.h>

using namespace std;
using glm::ivec2;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;


#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false

struct Camera {
  float focalLength;
  vec4 position;
  vec3 rotation;
  float movementSpeed;
  float rotationSpeed;
};

struct Pixel {
  int x;
  int y;
  float zinv;

  Pixel() : x(0), y(0), zinv(0.f) {};
  Pixel(glm::ivec2 vec, float zinv): x(vec.x), y(vec.y), zinv(zinv) {};
};

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(screen* screen);
void TransformationMatrix(mat4 &M);
void VertexShader(const vec4 &v, Pixel &p);
void Interpolate(Pixel a, Pixel b, vector<Pixel>& result);
void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result);
void DrawLineSDL(SDL_Surface* surface, Pixel a, Pixel b, vec3 color);
void DrawPolygonEdges(screen *screen, const vector<vec4> &vertices);
mat4 CalcRotationMatrix(vec3 rotation);
void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels);
void DrawRows(const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, vec3 color);
void DrawPolygon(screen *screen, const vector<vec4>& vertices, vec3 color);

vector<Triangle> triangles;
Camera camera;
float depthBuffer[SCREEN_HEIGHT][SCREEN_WIDTH];

int main(int argc, char* argv[]) {
  camera = {
    SCREEN_HEIGHT,
    vec4(0, 0, -3.001, 1),
    vec3(0, 0, 0),
    0.001,
    0.001,
  };

  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);
  
  LoadTestModel(triangles);

  // vector<ivec2> vertexPixels(3);
  // vertexPixels[0] = ivec2(10, 5);
  // vertexPixels[1] = ivec2(5, 10);
  // vertexPixels[2] = ivec2(15,15);
  // vector<ivec2> leftPixels;
  // vector<ivec2> rightPixels;
  // ComputePolygonRows(vertexPixels, leftPixels, rightPixels);
  // for (int row = 0; row < leftPixels.size(); ++row) {
  //   cout << "Start: ("
  //   << leftPixels[row].x << ","
  //   << leftPixels[row].y << "). "
  //   << "End: ("
  //   << rightPixels[row].x << ","
  //   << rightPixels[row].y << "). " << endl;
  // }

  while (NoQuitMessageSDL()) {
    Update();
    Draw(screen);
    SDL_Renderframe(screen);
  }

  SDL_SaveImage(screen, "screenshot.bmp");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen* screen) {
  /* Clear buffer */
  mat4 transMat;
  TransformationMatrix(transMat);
  memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));
  memset(depthBuffer, 0, SCREEN_HEIGHT*SCREEN_WIDTH*sizeof(float));
  
  for (uint32_t i = 0; i < triangles.size(); i++) {
    // vector<vec4> vertices({transMat * triangles[i].v0, transMat * triangles[i].v1, transMat * triangles[i].v2});
    vector<vec4> vertices({triangles[i].v0 - camera.position, triangles[i].v1 - camera.position, triangles[i].v2 - camera.position});

    // DrawPolygonEdges(screen, vertices);
    DrawPolygon(screen, vertices, triangles[i].color);
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

  vec3 tempRot = camera.rotation;

  const Uint8 *keystate = SDL_GetKeyboardState(NULL);
  if (keystate[SDL_SCANCODE_W]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x -= camera.rotationSpeed * dt;
    } else {
      camera.position.z += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_S]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.x += camera.rotationSpeed * dt;
    } else {
      camera.position.z -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_A]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y += camera.rotationSpeed * dt;
    } else {
      camera.position.x -= camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_D]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.y -= camera.rotationSpeed * dt;
    } else {
      camera.position.x += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_Q]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z += camera.rotationSpeed * dt;
    } else {
      camera.position.y += camera.movementSpeed * dt;
    }
  }

  if (keystate[SDL_SCANCODE_E]) {
    if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT]) {
      camera.rotation.z -= camera.rotationSpeed * dt;
    } else {
      camera.position.y -= camera.movementSpeed * dt;
    }
  }
}

void VertexShader(const vec4 &v, Pixel &p) {
  p.zinv = 1.f / v.z;
  p.x = (camera.focalLength * (v.x / v.z)) + (SCREEN_WIDTH / 2.f);
  p.y = (camera.focalLength * (v.y / v.z)) + (SCREEN_HEIGHT / 2.f);
}

void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result) {
  int N = result.size();
  vec2 step = vec2(b - a) / float(max(N-1,1));
  vec2 current(a);

  for(int i = 0; i < N; ++i) {
    result[i] = current;
    current += step;
  }
}

void Interpolate(Pixel a, Pixel b, vector<Pixel>& result) {
  int N = result.size();
  vec2 stepV = vec2(vec2(b.x, b.y) - vec2(a.x, a.y)) / float(max(N-1,1));
  float stepZ = b.zinv - a.zinv / float(max(N - 1, 1));
  vec2 currentV(vec2(a.x, a.y));
  float currentZ = a.zinv;

  for(int i = 0; i < N; ++i) {
    result[i] = Pixel(currentV, currentZ);
    currentV += stepV;
    currentZ += stepZ;
  }
}

void DrawLineSDL(screen* screen, Pixel a, Pixel b, vec3 color) {
  int dx = abs(a.x - b.x);
  int dy = abs(a.y - b.y);
  int pixels = max(dx, dy) + 1;
  vector<Pixel> line(pixels);
  Interpolate(a, b, line);

  for (Pixel coord : line) {
    PutPixelSDL(screen, coord.x, coord.y, color);
  }
}

void DrawPolygonEdges(screen *screen, const vector<vec4> &vertices) {
  int V = vertices.size();
  // Transform each vertex from 3D world position to 2D image position:
  vector<Pixel> projectedVertices(V);

  for (int i = 0; i < V; ++i) {
    VertexShader(vertices[i], projectedVertices[i]);
  }
  // Loop over all vertices and draw the edge from it to the next vertex:
  for (int i = 0; i < V; ++i) {
    int j = (i + 1) % V; // The next vertex
    vec3 color(1, 1, 1);
    DrawLineSDL(screen, projectedVertices[i], projectedVertices[j], color);
  }
}

mat4 CalcRotationMatrix(vec3 rotation) {
  float x = rotation.x;
  float y = rotation.y;
  float z = rotation.z;
  mat4 Rx = mat4(vec4(1, 0, 0, 0), vec4(0, cosf(x), sinf(x), 0), vec4(0, -sinf(x), cosf(x), 0), vec4(0, 0, 0, 1));
  mat4 Ry = mat4(vec4(cosf(y), 0, -sinf(y), 0), vec4(0, 1, 0, 0), vec4(sinf(y), 0, cosf(y), 0), vec4(0 , 0, 0, 1));
  mat4 Rz = mat4(vec4(cosf(z), sinf(z), 0, 0), vec4(-sinf(z), cosf(z), 0, 0), vec4(0, 0, 1, 0), vec4(0, 0, 0, 1));
  return Rz * Ry * Rx;
}

void TransformationMatrix(mat4 &M) {
  mat3 eye = mat3(1);
  mat4 rot = CalcRotationMatrix(camera.rotation);
  rot[3] = camera.position;
  mat4 transPos = mat4(vec4(eye[0], 0), vec4(eye[1], 0), vec4(eye[2], 0), camera.position);
  mat4 transNeg = 2.f * mat4(1) - transPos;
  M = transPos * rot * transNeg;
}


void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels) {
  // 1. Find max and min y-value of the polygon
  //    and compute the number of rows it occupies.
  int minY = +numeric_limits<int>::max(), maxY = -numeric_limits<int>::max();
  for (Pixel vertexPixel : vertexPixels) {
    minY = minY > vertexPixel.y ? vertexPixel.y : minY;
    maxY = maxY < vertexPixel.y ? vertexPixel.y : maxY;
  }
  int rows = maxY - minY + 1;

  // 2. Resize leftPixels and rightPixels
  //    so that they have an element for each row.

  // 3. Initialize the x-coordinates in leftPixels
  //    to some really large value and the x-coordinates
  //    in rightPixels to some really small value.
  leftPixels.reserve(rows);
  rightPixels.reserve(rows);
  for (int i = 0; i < rows; i++) {
    leftPixels.push_back(Pixel(ivec2(+numeric_limits<int>::max(), minY + i), 0.f));
    rightPixels.push_back(Pixel(ivec2(-numeric_limits<int>::max(), minY + i), 0.f));
  }

  // 4. Loop through all edges of the polygon and use
  //    linear interpolation to find the x-coordinate for
  //    each row it occupies. Update the corresponding
  //    values in rightPixels and leftPixels.
  for (int i = 0; i < vertexPixels.size(); ++i) {
    int j = (i + 1) % vertexPixels.size(); // The next vertex
    vector<Pixel> coords(rows);
    Interpolate(vertexPixels[i], vertexPixels[j], coords);
    for (Pixel coord : coords) {
      if (leftPixels[coord.y - minY].x > coord.x) {
        leftPixels[coord.y - minY].x = coord.x;
        leftPixels[coord.y - minY].zinv = coord.zinv;
      }
      if (rightPixels[coord.y - minY].x < coord.x) {
        rightPixels[coord.y - minY].x = coord.x;
        rightPixels[coord.y - minY].zinv = coord.zinv;
      }
    }
  }
}

void DrawRows(screen *screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels, vec3 color) {
  for (int i = 0; i < leftPixels.size(); i++) {
    int pixels = abs(rightPixels[i].x - leftPixels[i].x + 1);
    vector<Pixel> ps(pixels);
    Interpolate(rightPixels[i], leftPixels[i], ps);
    for (Pixel p : ps) {
      if (depthBuffer[p.y][p.x] < p.zinv) {
        PutPixelSDL(screen, p.x, p.y, color);
        depthBuffer[p.y][p.x] = p.zinv;
      }
    }
  }
}

void DrawPolygon(screen *screen, const vector<vec4>& vertices, vec3 color) {
    int V = vertices.size();
    vector<Pixel> vertexPixels(V);

    for(int i = 0; i < V; ++i) {
      VertexShader(vertices[i], vertexPixels[i]);
    }

    vector<Pixel> leftPixels;
    vector<Pixel> rightPixels;
    ComputePolygonRows(vertexPixels, leftPixels, rightPixels);
    DrawRows(screen, leftPixels, rightPixels, color);
}

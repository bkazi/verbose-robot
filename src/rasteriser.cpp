#include <iostream>
#include <glm/glm.hpp>
// #include <glm/gtx/string_cast.hpp>
#include <SDL.h>
#include "SDLauxiliary.h"
#include "TestModel.h"
#include "camera.h"
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

struct Pixel {
  int x;
  int y;
  float zinv;
  vec4 pos3d;
  vec4 normal;
  vec3 reflectance;

  Pixel() {};
  Pixel(ivec2 vec): x(vec.x), y(vec.y) {};
  Pixel(ivec2 vec, float zinv, vec4 pos3d, vec4 normal, vec3 reflectance): x(vec.x), y(vec.y), zinv(zinv), pos3d(pos3d), normal(normal), reflectance(reflectance) {};
};

struct Vertex {
  vec4 position;
  vec4 normal;
  vec3 reflectance;

  Vertex(vec4 position): position(position) {};
  Vertex(vec4 position, vec4 normal, vec3 reflectance): position(position), normal(normal), reflectance(reflectance) {};
};

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(screen* screen);
void VertexShader(const Vertex &v, Pixel &p);
void PixelShader(screen* screen, const Pixel &p);
void Interpolate(Pixel a, Pixel b, vector<Pixel>& result);
void Interpolate(ivec2 a, ivec2 b, vector<ivec2>& result);
void DrawLineSDL(SDL_Surface* surface, Pixel a, Pixel b, vec3 color);
void DrawPolygonEdges(screen *screen, const vector<vec4> &vertices);
mat4 CalcRotationMatrix(vec3 rotation);
void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels);
void DrawRows(const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels);
void DrawPolygon(screen *screen, const vector<Vertex>& vertices);

vector<Shape *> shapes;
Camera *camera;

vec4 lightPos(0, -0.5, -0.7, 1);
vec3 lightPower = 5.f * vec3(1, 1, 1);
vec3 indirectLightPowerPerArea = 0.5f * vec3(1, 1, 1);

int main(int argc, char* argv[]) {
  camera = new Camera(
    vec4(0, 0, -3.001, 1),
    vec3(0, 0, 0),
    SCREEN_HEIGHT,
    0.001,
    0.001
  );

  screen *screen = InitializeSDL(SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);
  
  LoadTestModel(shapes);

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
  mat4 transMat = camera->getTransformationMatrix();
  memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));
  memset(screen->depthBuffer, 0, screen->height*screen->width*sizeof(uint32_t));
  
  for (uint32_t i = 0; i < shapes.size(); i++) {
    Triangle *tri;
    if ((tri = dynamic_cast<Triangle *>(shapes[i]))) {
    vector<Vertex> vertices({
      Vertex(transMat * tri->v0, tri->getNormal(vec4(1)), tri->color),
      Vertex(transMat * tri->v1, tri->getNormal(vec4(1)), tri->color),
      Vertex(transMat * tri->v2, tri->getNormal(vec4(1)), tri->color)
    });

    // DrawPolygonEdges(screen, vertices);
    DrawPolygon(screen, vertices);
    } else {
      cout << "A Shape which isn't a Triangle is in the scene, skipping" << endl;
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

void VertexShader(const Vertex &v, Pixel &p) {
  vec4 pos = v.position;
  p.zinv = 1.f / pos.z;
  p.x = int(camera->focalLength * pos.x * p.zinv) + (SCREEN_WIDTH / 2);
  p.y = int(camera->focalLength * pos.y * p.zinv) + (SCREEN_HEIGHT / 2);
  p.pos3d = v.position;
  p.normal = v.normal;
  p.reflectance = v.reflectance;
}

void PixelShader(screen *screen, const Pixel &p) {
  mat4 transMat = camera->getTransformationMatrix();
  vec3 r = vec3(transMat * lightPos - p.pos3d);
  float rLen = glm::length(r);
  vec4 rNorm = vec4(r / rLen, 1);
  vec3 D = lightPower * max(glm::dot(rNorm, p.normal), 0.f) / float(4 * M_PI * powf(rLen, 2.f));
  vec3 illumination = p.reflectance * (D + indirectLightPowerPerArea);
  PutPixelSDL(screen, p.x, p.y, illumination, p.zinv);
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
  float s = 1.f / float(max(N - 1, 1));

  float stepX = float(b.x - a.x) * s;
  float currentX = a.x;

  float stepY = float(b.y - a.y) * s;
  float currentY = a.y;

  float stepZ = (b.zinv - a.zinv) * s;
  float currentZ = a.zinv;

  vec3 stepPos = (vec3(b.pos3d) * b.zinv - vec3(a.pos3d) * a.zinv) * s;
  vec3 currentPos = vec3(a.pos3d) * a.zinv;

  vec3 stepN = (vec3(b.normal) - vec3(a.normal)) * s;
  vec3 currentN = vec3(a.normal);

  for(int i = 0; i < N; ++i) {
    result[i] = Pixel(ivec2(round(currentX), round(currentY)), currentZ, vec4(currentPos / currentZ, 1), vec4(currentN, 1), a.reflectance);
    currentX += stepX;
    currentY += stepY;
    currentZ += stepZ;
    currentPos += stepPos;
    currentN += stepN;
  }
}

void DrawLineSDL(screen* screen, Pixel a, Pixel b, vec3 color) {
  int dx = abs(a.x - b.x);
  int dy = abs(a.y - b.y);
  int pixels = max(dx, dy) + 1;
  vector<Pixel> line(pixels);
  Interpolate(a, b, line);

  for (Pixel coord : line) {
    PixelShader(screen, coord);
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

void ComputePolygonRows(const vector<Pixel>& vertexPixels, vector<Pixel>& leftPixels, vector<Pixel>& rightPixels) {
  // 1. Find max and min y-value of the polygon
  //    and compute the number of rows it occupies.
  int minY = +numeric_limits<int>::max(), maxY = -numeric_limits<int>::max();
  for (Pixel vertexPixel : vertexPixels) {
    minY = minY > vertexPixel.y ? vertexPixel.y : minY;
    maxY = maxY < vertexPixel.y ? vertexPixel.y : maxY;
  }
  int rows = abs(maxY - minY) + 1;

  // 2. Resize leftPixels and rightPixels
  //    so that they have an element for each row.

  // 3. Initialize the x-coordinates in leftPixels
  //    to some really large value and the x-coordinates
  //    in rightPixels to some really small value.
  leftPixels.reserve(rows);
  rightPixels.reserve(rows);
  for (int i = 0; i < rows; i++) {
    leftPixels.push_back(Pixel(ivec2(+numeric_limits<int>::max(), minY + i)));
    rightPixels.push_back(Pixel(ivec2(-numeric_limits<int>::max(), minY + i)));
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
      // cout << coord.y << " " << minY << endl;
      assert((coord.y - minY) >= 0);
      if (leftPixels[coord.y - minY].x > coord.x) {
        leftPixels[coord.y - minY] = coord;
      }
      if (rightPixels[coord.y - minY].x < coord.x) {
        rightPixels[coord.y - minY] = coord;
      }
    }
  }
}

void DrawRows(screen *screen, const vector<Pixel>& leftPixels, const vector<Pixel>& rightPixels) {
  for (int i = 0; i < leftPixels.size(); i++) {
    int pixels = abs(rightPixels[i].x - leftPixels[i].x) + 1;
    vector<Pixel> ps(pixels);
    Interpolate(rightPixels[i], leftPixels[i], ps);
    for (Pixel p : ps) {
      PixelShader(screen, p);
    }
  }
}

void DrawPolygon(screen *screen, const vector<Vertex>& vertices) {
    int V = vertices.size();
    vector<Pixel> vertexPixels(V);

    for(int i = 0; i < V; ++i) {
      VertexShader(vertices[i], vertexPixels[i]);
    }

    vector<Pixel> leftPixels;
    vector<Pixel> rightPixels;
    ComputePolygonRows(vertexPixels, leftPixels, rightPixels);
    DrawRows(screen, leftPixels, rightPixels);
}

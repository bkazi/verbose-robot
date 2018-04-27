#include <SDL.h>
#include <assert.h>
#include <stdint.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <opencv/cv.hpp>

#include "camera.h"
#include "light.h"
#include "post_processing.h"
#include "scene.h"
#include "screen.h"
#include "util.h"

using namespace cv;
using namespace std;
using glm::ivec2;
using glm::mat3;
using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 1024
#define FULLSCREEN_MODE false
#define NEAR_PLANE 0.1
#define FAR_PLANE 5
#define ENABLE_FXAA
#define SMOOTH_SHADOWS

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS */

void Update(Camera *camera);
void Draw(screen *screen, Camera *camera);
void VertexShader(const Vertex &v, Pixel &p, mat4 transMat, mat4 projMat);
void PixelShader(screen *screen, const Pixel &p, const Primitive *primitive,
                 Camera *camera);
void Interpolate(Pixel a, Pixel b, vector<Pixel> &result);
void Interpolate(ivec2 a, ivec2 b, vector<ivec2> &result);
void DrawLineSDL(SDL_Surface *surface, Pixel a, Pixel b, vec3 color);
void DrawPolygonEdges(screen *screen, const vector<vec4> &vertices);
void ComputePolygonRows(const vector<Pixel> &vertexPixels,
                        vector<Pixel> &leftPixels, vector<Pixel> &rightPixels);
void DrawRows(const vector<Pixel> &leftPixels,
              const vector<Pixel> &rightPixels);
void DrawPolygon(screen *screen, const vector<Vertex> &vertices,
                 const Primitive *primitive, Camera *camera);
void ClipPolygon(vector<Pixel> &vertexPixels, int height, int width);
void DrawShadowMap(Light &light);
void FXAA(screen *s);

Scene *scene;
Light light;

vec4 lightPos(0, -0.5, -0.7, 1);
vec4 lightDir(-2.0, 7.2, 0.9, 0);
vec3 lightPower = 0.5f * vec3(1, 1, 1);
vec3 indirectLightPowerPerArea = 0.5f * vec3(1, 1, 1);

int main(int argc, char *argv[]) {
  scene = new Scene();

  Camera *camera = new Camera(vec4(0, 0, -3, 1), vec3(0, M_PI_2, M_PI),
                              SCREEN_HEIGHT, 0.001, 0.001);

  light.position = lightPos;
  light.direction = lightDir;
  light.power = lightPower;
  light.needsUpdate = true;

  screen *screen =
      InitializeSDL("rasteriser", SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE);

  if (argc >= 2) {
    const string path = argv[1];
    scene->LoadModel(path);
  } else {
    scene->LoadTest();
  }

  while (NoQuitMessageSDL()) {
    Update(camera);
    DrawShadowMap(light);
    Draw(screen, camera);
    // for (int y = 0; y < LIGHTMAP_SIZE; y++) {
    //   for (int x = 0; x < LIGHTMAP_SIZE; x++) {
    //     PutPixelSDL(screen, x, y, vec3(light.depthBuffer[y * LIGHTMAP_SIZE +
    //     x]), 100.f);
    //   }
    // }
#ifdef ENABLE_FXAA
    FXAA(screen);
#endif
    SDL_Renderframe(screen);
  }

  SDL_SaveImage(screen, "screenshot.png");

  KillSDL(screen);
  return 0;
}

/*Place your drawing here*/
void Draw(screen *screen, Camera *camera) {
  /* Clear buffer */
  clear(screen);

#pragma omp parallel for simd schedule(guided)
  for (uint32_t i = 0; i < scene->objects.size(); ++i) {
    for (uint32_t j = 0; j < scene->objects[i]->primitives.size(); ++j) {
      Triangle *tri;
      if ((tri = dynamic_cast<Triangle *>(scene->objects[i]->primitives[j])) !=
          nullptr) {
        vector<Vertex> vertices({tri->v0, tri->v1, tri->v2});
        DrawPolygon(screen, vertices, tri, camera);
      }
    }
  }
}

/*Place updates of parameters here*/
void Update(Camera *camera) {
  static int t = SDL_GetTicks();
  /* Compute frame time */
  int t2 = SDL_GetTicks();
  float dt = float(t2 - t);
  t = t2;
  cout << "Render time: " << dt << " ms." << endl;
  /* Update variables*/

  light.update(dt);
  camera->update(dt);
}

void VertexShader(const Vertex &v, Pixel &p, mat4 transMat, mat4 projMat) {
  vec4 pos = projMat * transMat * v.position;
  p.homogenous = pos;
  p.zinv = 1.f / (transMat * v.position).z;
  p.worldPos = v.position;
  p.normal = glm::normalize(transMat * v.normal);
  p.uv = v.uv;
  p.reflectance = v.color;
}

void createCoordinateSystem(const vec3 &N, vec3 &Nt, vec3 &Nb) {
  if (fabs(N.x) > fabs(N.y))
    Nt = glm::normalize(vec3(N.z, 0, -N.x));
  else
    Nt = glm::normalize(vec3(0, -N.z, N.y));
  Nb = glm::cross(N, Nt);
}

void PixelShader(screen *screen, const Pixel &p, const Primitive *primitive,
                 Camera *camera) {
  vec3 color;
  if (primitive->material.diffuseTexture != nullptr) {
    color = primitive->material.diffuseTexture->sample(p.uv);
  } else {
    color = p.reflectance;
  }
  vec4 normal;
  if (primitive->material.bumpTexture != nullptr) {
    vec3 Nt, Nb;
    vec3 sample =
        (2.f * primitive->material.diffuseTexture->sample(p.uv)) - 1.f;
    createCoordinateSystem(vec3(p.normal), Nt, Nb);
    normal = vec4(mat3(Nb, vec3(p.normal), Nt) * sample, 0.f);
  } else {
    normal = p.normal;
  }

  vec3 D;
  if (light.test(p.worldPos)) {
    mat4 transMat = camera->getTransformationMatrix();
    vec4 dir = glm::normalize(transMat * light.direction);
    D = light.power * max(glm::dot(dir, normal), 0.f);
  } else {
    D = vec3(0.f);
  }
  vec3 illumination = color * (D + indirectLightPowerPerArea);
  PutPixelSDL(screen, p.x, p.y, illumination, p.zinv);
}

float edgeFunctionHom(Pixel a, Pixel b, Pixel p) {
  int ax = a.homogenous.x / a.homogenous.w;
  int ay = a.homogenous.y / a.homogenous.w;
  int bx = b.homogenous.x / b.homogenous.w;
  int by = b.homogenous.y / b.homogenous.w;
  return (p.homogenous.x / p.homogenous.w - ax) * (by - ay) -
         (p.homogenous.y / p.homogenous.w - ay) * (bx - ax);
}

float edgeFunction(Pixel a, Pixel b, Pixel p) {
  return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

void InterpolateBarycentric(vector<Pixel> vertexPixels, float w[3], Pixel &p) {
  p.reflectance = vertexPixels[0].reflectance;
  p.zinv = w[0] * vertexPixels[0].zinv + w[1] * vertexPixels[1].zinv +
           w[2] * vertexPixels[2].zinv;
  p.worldPos = (w[0] * vertexPixels[0].worldPos * vertexPixels[0].zinv +
                w[1] * vertexPixels[1].worldPos * vertexPixels[1].zinv +
                w[2] * vertexPixels[2].worldPos * vertexPixels[2].zinv) /
               p.zinv;
  p.normal =
      glm::normalize((w[0] * vertexPixels[0].normal * vertexPixels[0].zinv +
                      w[1] * vertexPixels[1].normal * vertexPixels[1].zinv +
                      w[2] * vertexPixels[2].normal * vertexPixels[2].zinv) /
                     p.zinv);

  p.uv = (w[0] * vertexPixels[0].uv * vertexPixels[0].zinv +
          w[1] * vertexPixels[1].uv * vertexPixels[1].zinv +
          w[2] * vertexPixels[2].uv * vertexPixels[2].zinv) /
         p.zinv;
}

Pixel ClipEdge(Pixel i, Pixel j, vector<Pixel> vertexPixels, float a) {
  assert(vertexPixels.size() == 3);

  float w[3];
  Pixel p;
  p.homogenous = (1 - a) * i.homogenous + a * j.homogenous;
  float areaDen =
      1 / edgeFunctionHom(vertexPixels[2], vertexPixels[1], vertexPixels[0]);

  w[0] = edgeFunctionHom(vertexPixels[2], vertexPixels[1], p) * areaDen;
  w[1] = edgeFunctionHom(vertexPixels[0], vertexPixels[2], p) * areaDen;
  w[2] = edgeFunctionHom(vertexPixels[1], vertexPixels[0], p) * areaDen;

  InterpolateBarycentric(vertexPixels, w, p);

  return p;
}

void ClipPolygon(vector<Pixel> &vertexPixels, int height, int width) {
  vector<Pixel> output = vertexPixels;
  vector<Pixel> outputTemp;

  // Z - axis
  for (int i = 0; i < output.size(); ++i) {
    int j = (i + 1) % output.size();  // The next vertex
    float ip1 = output[i].homogenous.z - NEAR_PLANE;
    float ip2 =
        output[i].homogenous.w * FAR_PLANE * height - output[i].homogenous.z;
    float jp2 =
        output[j].homogenous.w * FAR_PLANE * height - output[j].homogenous.z;
    float jp1 = output[j].homogenous.z - NEAR_PLANE;

    if (ip1 >= 0 && ip2 >= 0 && jp1 >= 0 && jp2 >= 0) {
      outputTemp.push_back(output[j]);
    } else if (ip1 >= 0 && ip2 < 0 && jp1 >= 0 && jp2 >= 0) {
      float a = jp2 / (jp2 - ip2);
      outputTemp.push_back(ClipEdge(output[j], output[i], vertexPixels, a));
      outputTemp.push_back(output[j]);
    } else if (ip1 < 0 && ip2 >= 0 && jp1 >= 0 && jp2 >= 0) {
      float a = jp1 / (jp1 - ip1);
      outputTemp.push_back(ClipEdge(output[j], output[i], vertexPixels, a));
      outputTemp.push_back(output[j]);
    } else if (jp1 >= 0 && jp2 < 0 && ip1 >= 0 && ip2 >= 0) {
      float a = ip2 / (ip2 - jp2);
      outputTemp.push_back(ClipEdge(output[i], output[j], vertexPixels, a));
    } else if (jp1 < 0 && jp2 >= 0 && ip1 >= 0 && ip2 >= 0) {
      float a = ip1 / (ip1 - jp1);
      outputTemp.push_back(ClipEdge(output[i], output[j], vertexPixels, a));
    }
  }

  output.swap(outputTemp);
  outputTemp.clear();

  // Y - axis
  for (int i = 0; i < output.size(); ++i) {
    int j = (i + 1) % output.size();  // The next vertex
    float ip1 =
        output[i].homogenous.w * (height / 2.f - 1) - output[i].homogenous.y;
    float ip2 =
        output[i].homogenous.w * (height / 2.f) + output[i].homogenous.y;
    float jp2 =
        output[j].homogenous.w * (height / 2.f) + output[j].homogenous.y;
    float jp1 =
        output[j].homogenous.w * (height / 2.f - 1) - output[j].homogenous.y;

    if (ip1 >= 0 && ip2 >= 0 && jp1 >= 0 && jp2 >= 0) {
      outputTemp.push_back(output[j]);
    } else if (ip1 >= 0 && ip2 < 0 && jp1 >= 0 && jp2 >= 0) {
      float a = jp2 / (jp2 - ip2);
      outputTemp.push_back(ClipEdge(output[j], output[i], vertexPixels, a));
      outputTemp.push_back(output[j]);
    } else if (ip1 < 0 && ip2 >= 0 && jp1 >= 0 && jp2 >= 0) {
      float a = jp1 / (jp1 - ip1);
      outputTemp.push_back(ClipEdge(output[j], output[i], vertexPixels, a));
      outputTemp.push_back(output[j]);
    } else if (jp1 >= 0 && jp2 < 0 && ip1 >= 0 && ip2 >= 0) {
      float a = ip2 / (ip2 - jp2);
      outputTemp.push_back(ClipEdge(output[i], output[j], vertexPixels, a));
    } else if (jp1 < 0 && jp2 >= 0 && ip1 >= 0 && ip2 >= 0) {
      float a = ip1 / (ip1 - jp1);
      outputTemp.push_back(ClipEdge(output[i], output[j], vertexPixels, a));
    }
  }

  output.swap(outputTemp);
  outputTemp.clear();

  // X - axis
  for (int i = 0; i < output.size(); ++i) {
    int j = (i + 1) % output.size();  // The next vertex
    float ip1 =
        output[i].homogenous.w * (width / 2.f - 1) - output[i].homogenous.x;
    float ip2 = output[i].homogenous.w * (width / 2.f) + output[i].homogenous.x;
    float jp2 = output[j].homogenous.w * (width / 2.f) + output[j].homogenous.x;
    float jp1 =
        output[j].homogenous.w * (width / 2.f - 1) - output[j].homogenous.x;

    if (ip1 >= 0 && ip2 >= 0 && jp1 >= 0 && jp2 >= 0) {
      outputTemp.push_back(output[j]);
    } else if (ip1 >= 0 && ip2 < 0 && jp1 >= 0 && jp2 >= 0) {
      float a = jp2 / (jp2 - ip2);
      outputTemp.push_back(ClipEdge(output[j], output[i], vertexPixels, a));
      outputTemp.push_back(output[j]);
    } else if (ip1 < 0 && ip2 >= 0 && jp1 >= 0 && jp2 >= 0) {
      float a = jp1 / (jp1 - ip1);
      outputTemp.push_back(ClipEdge(output[j], output[i], vertexPixels, a));
      outputTemp.push_back(output[j]);
    } else if (jp1 >= 0 && jp2 < 0 && ip1 >= 0 && ip2 >= 0) {
      float a = ip2 / (ip2 - jp2);
      outputTemp.push_back(ClipEdge(output[i], output[j], vertexPixels, a));
    } else if (jp1 < 0 && jp2 >= 0 && ip1 >= 0 && ip2 >= 0) {
      float a = ip1 / (ip1 - jp1);
      outputTemp.push_back(ClipEdge(output[i], output[j], vertexPixels, a));
    }
  }

  vertexPixels = outputTemp;
}

void GetBounds(vector<Pixel> vertexPixels, int &minX, int &minY, int &maxX,
               int &maxY) {
  assert(vertexPixels.size() > 0);
  minY = +numeric_limits<int>::max();
  minX = +numeric_limits<int>::max();
  maxY = -numeric_limits<int>::max();
  maxX = -numeric_limits<int>::max();

  for (int i = 0; i < vertexPixels.size(); ++i) {
    if (minX > vertexPixels[i].x) {
      minX = vertexPixels[i].x;
    }
    if (minY > vertexPixels[i].y) {
      minY = vertexPixels[i].y;
    }
    if (maxX < vertexPixels[i].x) {
      maxX = vertexPixels[i].x;
    }
    if (maxY < vertexPixels[i].y) {
      maxY = vertexPixels[i].y;
    }
  }
}

void FillTriangles(screen *screen, vector<Pixel> vertexPixels,
                   const Primitive *primitive, Camera *camera) {
  assert(vertexPixels.size() == 3);

  int minX, minY, maxX, maxY;
  Pixel p;
  float w[3];
  bool inside;

  GetBounds(vertexPixels, minX, minY, maxX, maxY);
  assert(minX != +numeric_limits<int>::max());
  assert(minY != +numeric_limits<int>::max());
  assert(maxX != -numeric_limits<int>::max());
  assert(maxY != -numeric_limits<int>::max());

  float areaDen =
      1 / edgeFunction(vertexPixels[2], vertexPixels[1], vertexPixels[0]);
#pragma omp simd
  for (int x = minX; x <= maxX; x++) {
    for (int y = minY; y <= maxY; y++) {
      p.x = x;
      p.y = y;
      w[0] = edgeFunction(vertexPixels[2], vertexPixels[1], p) * areaDen;
      w[1] = edgeFunction(vertexPixels[0], vertexPixels[2], p) * areaDen;
      w[2] = edgeFunction(vertexPixels[1], vertexPixels[0], p) * areaDen;
      inside = true;
      for (int i = 0; i < 3; ++i) {
        inside &= w[i] >= 0;
      }
      if (inside == true) {
        InterpolateBarycentric(vertexPixels, w, p);
        PixelShader(screen, p, primitive, camera);
      }
    }
  }
}

void DrawPolygon(screen *screen, const vector<Vertex> &vertices,
                 const Primitive *primitive, Camera *camera) {
  int V = vertices.size();
  vector<Pixel> vertexPixels(V);

  mat4 transMat = camera->getTransformationMatrix();
  // TransformationMatrix(camera->rotation, camera->position, transMat);
  mat4 projMat = mat4(1);
  // projMat[2].z = (- NEAR_PLANE - FAR_PLANE) / (NEAR_PLANE - FAR_PLANE);
  projMat[2].w = 1 / camera->focalLength;
  // projMat[3].z = (2 * FAR_PLANE * NEAR_PLANE) / (NEAR_PLANE - FAR_PLANE);
  projMat[3].w = 0;
  for (int i = 0; i < V; ++i) {
    VertexShader(vertices[i], vertexPixels[i], transMat, projMat);
  }

  ClipPolygon(vertexPixels, screen->height, screen->width);
  // homogenous divide
  for (int i = 0; i < vertexPixels.size(); i++) {
    vertexPixels[i].homogenous /= vertexPixels[i].homogenous.w;
    vertexPixels[i].x = round(vertexPixels[i].homogenous.x + screen->width / 2);
    vertexPixels[i].y =
        round(vertexPixels[i].homogenous.y + screen->height / 2);
  }

  if (vertexPixels.size() < 3) {
    return;
  }

  if (vertexPixels.size() >= 4) {
    for (int i = 1; i < vertexPixels.size() - 1; i++) {
      vector<Pixel> verts;
      verts.push_back(vertexPixels[0]);
      verts.push_back(vertexPixels[i]);
      verts.push_back(vertexPixels[i + 1]);
      FillTriangles(screen, verts, primitive, camera);
    }
  } else {
    FillTriangles(screen, vertexPixels, primitive, camera);
  }
}

void DrawShadowMap(Light &light) {
  if (light.needsUpdate == true) {
    screen *s = createScreen("rasteriser", LIGHTMAP_SIZE, LIGHTMAP_SIZE);

    Camera *camera =
        new Camera(light.position, vec3(light.direction), LIGHTMAP_SIZE, 0, 0);

    Draw(s, camera);
#ifdef SMOOTH_SHADOWS
    Mat depthBuffer = cvUnpackDepthBuffer(s);
    double min, max;
    minMaxLoc(depthBuffer, &min, &max);
    depthBuffer.convertTo(depthBuffer, CV_8UC3, 255.0 / (max - min),
                          -255.0 * min / (max - min));
    Mat blurredDepth;
    GaussianBlur(depthBuffer, blurredDepth, Size(27, 27), 0, 0);
    blurredDepth.convertTo(blurredDepth, CV_32F, (max - min) / 255.0,
                           255.0 * min / (max - min));
#pragma omp parallel for collapse(2)
    for (int x = 0; x < s->width; x++) {
      for (int y = 0; y < s->height; y++) {
        s->depthBuffer[y * s->width + x] = blurredDepth.at<float>(Point(x, y));
      }
    }
#endif

    memcpy(light.depthBuffer, s->depthBuffer,
           LIGHTMAP_SIZE * LIGHTMAP_SIZE * sizeof(float));
  }
  light.needsUpdate = false;
}

void FXAA(screen *s) {
  Mat frameBuffer = cvUnpackToMat(s);
  Mat depthBuffer = cvUnpackDepthBuffer(s);
  findEdges(depthBuffer);
  double min, max;
  minMaxLoc(depthBuffer, &min, &max);
  depthBuffer.convertTo(depthBuffer, CV_8UC3, 255.0 / (max - min),
                        -255.0 * min / (max - min));
  Mat blurredImage, blurredMask;

  GaussianBlur(frameBuffer, blurredImage, Size(3, 3), 0, 0);
  GaussianBlur(depthBuffer, blurredMask, Size(3, 3), 0, 0);
  maskImage(frameBuffer, blurredImage, blurredMask, frameBuffer);
  cvPackToScreen(s, frameBuffer);
}

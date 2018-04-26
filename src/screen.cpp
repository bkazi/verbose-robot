#include "screen.h"

#include <glm/glm.hpp>
#include <iostream>
#include "SDL.h"

using namespace std;
using namespace cv;
using glm::clamp;
using glm::vec3;

void SDL_SaveImage(screen* s, const char* filename) {
  Mat mat = cvUnpackToMat(s);
  imwrite(filename, mat);
}

void KillSDL(screen* s) {
  delete[] s->pixels;
  delete[] s->depthBuffer;
  SDL_DestroyTexture(s->texture);
  SDL_DestroyRenderer(s->renderer);
  SDL_DestroyWindow(s->window);
  SDL_Quit();
}

void SDL_Renderframe(screen* s) {
  uint32_t *buffer = new uint32_t[s->width * s->height];
#pragma omp parallel for collapse(2)
  for (int y = 0; y < s->height; y++) {
    for (int x = 0; x < s->width; x++) {
      glm::vec3 color = s->pixels[y * s->width + x] /
                        (float)(s->samples > 0 ? s->samples : 1.f);
      uint32_t r = uint32_t(clamp(255 * color.r, 0.f, 255.f));
      uint32_t g = uint32_t(clamp(255 * color.g, 0.f, 255.f));
      uint32_t b = uint32_t(clamp(255 * color.b, 0.f, 255.f));

      buffer[y * s->width + x] = (0xFF << 24) + (r << 16) + (g << 8) + b;
    }
  }
  SDL_UpdateTexture(s->texture, NULL, buffer, s->width * sizeof(uint32_t));
  SDL_RenderClear(s->renderer);
  SDL_RenderCopy(s->renderer, s->texture, NULL, NULL);
  SDL_RenderPresent(s->renderer);
}

screen* createScreen(string type, int width, int height) {
  screen* s = new screen;
  s->width = width;
  s->height = height;
  if (type == "raytracer") {
    s->accumulate = true;
  } else {
    s->accumulate = false;
  }
  s->pixels = new vec3[width * height];
  s->depthBuffer = new float[width * height];
  s->samples = 0;

  clear(s);

  return s;
}

screen* InitializeSDL(string type, int width, int height, bool fullscreen) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    std::cout << "Could not initialise SDL: " << SDL_GetError() << std::endl;
    exit(1);
  }

  screen *s = createScreen(type, width, height);

  uint32_t flags = SDL_WINDOW_OPENGL;
  if (fullscreen) {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }
  s->window = SDL_CreateWindow("COMS30115", SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED, width, height, flags);
  if (s->window == 0) {
    std::cout << "Could not set video mode: " << SDL_GetError() << std::endl;
    exit(1);
  }

  flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  s->renderer = SDL_CreateRenderer(s->window, -1, flags);
  if (s->renderer == 0) {
    std::cout << "Could not create renderer: " << SDL_GetError() << std::endl;
    exit(1);
  }
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_RenderSetLogicalSize(s->renderer, width, height);

  s->texture = SDL_CreateTexture(s->renderer, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STATIC, s->width, s->height);
  if (s->texture == 0) {
    std::cout << "Could not allocate texture: " << SDL_GetError() << std::endl;
    exit(1);
  }

  return s;
}

bool NoQuitMessageSDL() {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
      return false;
    }
    if (e.type == SDL_KEYDOWN) {
      if (e.key.keysym.sym == SDLK_ESCAPE) {
        return false;
      }
    }
  }
  return true;
}

void RaytracerPutPixelSDL(screen* s, int x, int y, vec3 color, float samples) {
  s->pixels[y * s->width + x] += color;
  s->samples = samples;
}

void RasteriserPutPixelSDL(screen* s, int x, int y, vec3 color, float depth) {
  if (depth > s->depthBuffer[y * s->width + x]) {
    s->depthBuffer[y * s->width + x] = depth;
    s->pixels[y * s->width + x] = color;
  }
}

void PutPixelSDL(screen* s, int x, int y, vec3 color, float SorD) {
  if (x < 0 || x >= s->width || y < 0 || y >= s->height) {
    std::cout << "apa" << std::endl;
    return;
  }
  if (s->accumulate == true) {
    return RaytracerPutPixelSDL(s, x, y, color, (int)SorD);
  } else {
    return RasteriserPutPixelSDL(s, x, y, color, SorD);
  }
}

Mat cvUnpackToMat(screen* s) {
  Mat mat(s->height, s->width, CV_8UC3, Scalar(0, 0, 0));
#pragma omp parallel for collapse(2)
  for (uint32_t y = 0; y < s->height; ++y) {
    for (uint32_t x = 0; x < s->width; ++x) {
      vec3 pixel = s->pixels[y * s->width + x];

      Vec3b color = mat.at<Vec3b>(Point(x, y));
      color[0] = clamp(pixel.b * 255.f, 0.f, 255.f);
      color[1] = clamp(pixel.g * 255.f, 0.f, 255.f);
      color[2] = clamp(pixel.r * 255.f, 0.f, 255.f);
      mat.at<Vec3b>(Point(x, y)) = color;
    }
  }
  return mat;
}

void cvPackToScreen(screen* s, Mat mat) {
#pragma omp parallel for collapse(2)
  for (uint32_t y = 0; y < s->height; ++y) {
    for (uint32_t x = 0; x < s->width; ++x) {
      Vec3b color = mat.at<Vec3b>(Point(x, y));

      float r = clamp((float)color[2] / 255.f, 0.f, 1.f);
      float g = clamp((float)color[1] / 255.f, 0.f, 1.f);
      float b = clamp((float)color[0] / 255.f, 0.f, 1.f);

      s->pixels[y * s->width + x] = vec3(r, g, b);
    }
  }
}

void clear(screen* s) {
  memset(s->pixels, 0, s->height * s->width * sizeof(vec3));
  memset(s->depthBuffer, 0.f,
         s->height * s->width * sizeof(float));
}

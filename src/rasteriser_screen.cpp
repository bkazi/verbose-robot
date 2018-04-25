#include "rasteriser_screen.h"

#include <glm/glm.hpp>
#include <iostream>
#include "SDL.h"

using namespace cv;

void SDL_SaveImage(screen* s, const char* filename) {
  Mat mat = cvUnpackToMat(s);
  imwrite("screenshot.png", mat);
  exit(0);
}

void KillSDL(screen* s) {
  delete[] s->buffer;
  delete[] s->depthBuffer;
  SDL_DestroyTexture(s->texture);
  SDL_DestroyRenderer(s->renderer);
  SDL_DestroyWindow(s->window);
  SDL_Quit();
}

void SDL_Renderframe(screen* s) {
  SDL_UpdateTexture(s->texture, NULL, s->buffer, s->width * sizeof(uint32_t));
  SDL_RenderClear(s->renderer);
  SDL_RenderCopy(s->renderer, s->texture, NULL, NULL);
  SDL_RenderPresent(s->renderer);
}

screen* InitializeSDL(int width, int height, bool fullscreen) {
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    std::cout << "Could not initialise SDL: " << SDL_GetError() << std::endl;
    exit(1);
  }

  screen* s = new screen;
  s->width = width;
  s->height = height;
  s->buffer = new uint32_t[width * height];
  s->depthBuffer = new float[width * height];
  memset(s->buffer, 0, width * height * sizeof(uint32_t));
  memset(s->depthBuffer, 0.f, width * height * sizeof(float));

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

void PutPixelSDL(screen* s, int x, int y, glm::vec3 colour, float depth) {
  if (x < 0 || x >= s->width || y < 0 || y >= s->height) {
    std::cout << "apa" << std::endl;
    return;
  }
  uint32_t r = uint32_t(glm::clamp(255 * colour.r, 0.f, 255.f));
  uint32_t g = uint32_t(glm::clamp(255 * colour.g, 0.f, 255.f));
  uint32_t b = uint32_t(glm::clamp(255 * colour.b, 0.f, 255.f));

  if (depth > s->depthBuffer[y * s->width + x]) {
    s->depthBuffer[y * s->width + x] = depth;
    s->buffer[y * s->width + x] = (255 << 24) + (r << 16) + (g << 8) + b;
  }
}

Mat cvUnpackToMat(screen* s) {
  #pragma omp parallel for collapse(2)
  Mat mat(s->height, s->width, CV_8UC3, Scalar(0, 0, 0));
  for (uint32_t y = 0; y < s->height; ++y) {
    for (uint32_t x = 0; x < s->width; ++x) {
      uint32_t pixel = s->buffer[y * s->width + x];
      uint32_t mask = 0xFF;
      uint32_t b = mask & pixel;
      uint32_t g = mask & (pixel >> 8);
      uint32_t r = mask & (pixel >> 16);

      Vec3b color = mat.at<Vec3b>(Point(x, y));
      color[0] = b;
      color[1] = g;
      color[2] = r;
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

      uint32_t r = uint32_t(glm::clamp((float)color[2], 0.f, 255.f));
      uint32_t g = uint32_t(glm::clamp((float)color[1], 0.f, 255.f));
      uint32_t b = uint32_t(glm::clamp((float)color[0], 0.f, 255.f));

      s->buffer[y * s->width + x] = (255 << 24) + (r << 16) + (g << 8) + b;
    }
  }
}

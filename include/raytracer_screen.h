#ifndef RAYTRACER_SCREEN_H
#define RAYTRACER_SCREEN_H

#include <glm/glm.hpp>
#include "SDL.h"

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  int height;
  int width;
  uint32_t *buffer;
  glm::vec3* pixels;
  float *depthBuffer;
  int samples;
} screen;

screen *InitializeSDL(int width, int height, bool fullscreen = false);
bool NoQuitMessageSDL();
void PutPixelSDL(screen *s, int x, int y, glm::vec3 color, int samples);
void SDL_Renderframe(screen *s);
void KillSDL(screen *s);
void SDL_SaveImage(screen *s, const char *filename);

#endif

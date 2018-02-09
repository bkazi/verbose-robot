#ifndef SDL_AUXILIARY_H
#define SDL_AUXILIARY_H

#include "SDL.h"
#include <glm/glm.hpp>

typedef struct{
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  int height;
  int width;
  uint32_t *buffer;
  glm::vec3* pixels;
  int samples;
} screen;

screen* InitializeSDL( int width, int height, bool fullscreen = false );
bool NoQuitMessageSDL();
void PutPixelSDL( screen *s, int x, int y, glm::vec3 color, int samples );
void SDL_Renderframe(screen *s);
void KillSDL(screen* s);
void SDL_SaveImage(screen *s, const char* filename);

#endif

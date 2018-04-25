#ifndef RASTERISER_SCREEN_H
#define RASTERISER_SCREEN_H

#include <glm/glm.hpp>
#include <opencv/cv.hpp>
#include "SDL.h"

typedef struct {
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  int height;
  int width;
  uint32_t *buffer;
  float *depthBuffer;
} screen;

screen *InitializeSDL(int width, int height, bool fullscreen = false);
bool NoQuitMessageSDL();
void PutPixelSDL(screen *s, int x, int y, glm::vec3 color, float depth);
void SDL_Renderframe(screen *s);
void KillSDL(screen *s);
void SDL_SaveImage(screen *s, const char *filename);
cv::Mat cvUnpackToMat(screen *s);
void cvPackToScreen(screen *screen, cv::Mat mat);
#endif

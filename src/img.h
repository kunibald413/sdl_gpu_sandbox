#ifndef DEMO_IMG_H
#include <SDL3/SDL.h>

#define BYTES_PER_PIXEL 4
SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, const char *path);
#endif // !DEMO_IMG_H


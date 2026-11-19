#ifndef DEMO_SHADERS_H
#define DEMO_SHADERS_H

#include <SDL3/SDL.h>

SDL_GPUShader* LoadShader(
	SDL_GPUDevice* device,
	const char* shaderFilename,
	Uint32 samplerCount,
	Uint32 uniformBufferCount,
	Uint32 storageBufferCount,
	Uint32 storageTextureCount
);

#endif // !DEMO_SHADERS_H
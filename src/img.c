#include <SDL3/SDL.h>
#include "img.h"

SDL_GPUTexture *LoadTexture(SDL_GPUDevice *device, const char *path)
{
    SDL_Surface *surface = SDL_LoadSurface(path);
    if (!surface) {
        SDL_Log("Failed to load image '%s': %s", path, SDL_GetError());
        return NULL;
    }

    SDL_Surface *converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
    SDL_DestroySurface(surface);
    if (!converted) {
        SDL_Log("Failed to convert surface to RGBA8: %s", SDL_GetError());
        return NULL;
    }

    int tex_w = converted->w;
    int tex_h = converted->h;
    SDL_Log("Loaded texture: %s (%dx%d)", path, tex_w, tex_h);

    SDL_GPUTextureCreateInfo tex_info;
    SDL_zero(tex_info);
    tex_info.type                 = SDL_GPU_TEXTURETYPE_2D;
    tex_info.format               = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB;
    tex_info.usage                = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tex_info.width                = tex_w;
    tex_info.height               = tex_h;
    tex_info.layer_count_or_depth = 1;
    tex_info.num_levels           = 1;

    SDL_GPUTexture *gpu_texture = SDL_CreateGPUTexture(device, &tex_info);
    if (!gpu_texture) {
        SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
        SDL_DestroySurface(converted);
        return NULL;
    }

    Uint32 bytes_per_row   = (Uint32)tex_w * BYTES_PER_PIXEL;
    Uint32 total_bytes     = bytes_per_row * (Uint32)tex_h;

    SDL_GPUTransferBufferCreateInfo transfer_info;
    SDL_zero(transfer_info);
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size  = total_bytes;

    SDL_GPUTransferBuffer *transfer = SDL_CreateGPUTransferBuffer(
        device, &transfer_info);
    if (!transfer) {
        SDL_Log("Failed to create texture transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTexture(device, gpu_texture);
        SDL_DestroySurface(converted);
        return NULL;
    }

    void *mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (!mapped) {
        SDL_Log("Failed to map texture transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTexture(device, gpu_texture);
        SDL_DestroySurface(converted);
        return NULL;
    }

    const Uint8 *src_pixels = (const Uint8 *)converted->pixels;
    Uint8 *dst_pixels = (Uint8 *)mapped;
    for (int y = 0; y < tex_h; y++) {
        SDL_memcpy(
            dst_pixels + y * bytes_per_row,
            src_pixels + y * converted->pitch,
            bytes_per_row);
    }

    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer *upload_cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!upload_cmd) {
        SDL_Log("Failed to acquire command buffer for texture upload: %s",
                SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTexture(device, gpu_texture);
        SDL_DestroySurface(converted);
        return NULL;
    }

    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmd);
    if (!copy_pass) {
        SDL_Log("Failed to begin copy pass for texture upload: %s",
                SDL_GetError());
        SDL_CancelGPUCommandBuffer(upload_cmd);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        SDL_ReleaseGPUTexture(device, gpu_texture);
        SDL_DestroySurface(converted);
        return NULL;
    }

    SDL_GPUTextureTransferInfo tex_src;
    SDL_zero(tex_src);
    tex_src.transfer_buffer = transfer;
    tex_src.offset          = 0;
    tex_src.pixels_per_row  = tex_w;
    tex_src.rows_per_layer  = tex_h;

    SDL_GPUTextureRegion tex_dst;
    SDL_zero(tex_dst);
    tex_dst.texture = gpu_texture;
    tex_dst.w       = tex_w;
    tex_dst.h       = tex_h;
    tex_dst.d       = 1;

    SDL_UploadToGPUTexture(copy_pass, &tex_src, &tex_dst, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer);
    SDL_DestroySurface(converted);

    return gpu_texture;
}
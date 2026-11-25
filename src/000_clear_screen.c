
#include <SDL3/SDL.h>
#include "example_base.h"

typedef struct ClearData {
    SDL_FColor Color;
} ClearData;

static int Init(Context *c) {
    ClearData* data = SDL_calloc(1, sizeof(ClearData));
    if (!data) return 1;
    data->Color = (SDL_FColor) {0.1, 0.3, 0.55, 1.0};
    c->ExampleData = data;
    return 0;
}

static int Update(Context *c) {
    (void)c;
    return 0;
}

static int Draw(Context *c) {
    ClearData* data = (ClearData *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    SDL_GPUTexture *swapchain = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, NULL, NULL))
        return -1;

    if (swapchain) {
        SDL_GPUColorTargetInfo cti = {0};
        cti.texture     = swapchain;
        cti.clear_color = data->Color;
        cti.load_op     = SDL_GPU_LOADOP_CLEAR;
        cti.store_op    = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &cti, 1, NULL);
        SDL_EndGPURenderPass(pass);
    }
    SDL_SubmitGPUCommandBuffer(cmd);

    return 0;
}

static void Quit(Context *c) {
    ClearData* data = c->ExampleData;
    SDL_free(data);
    c->ExampleData = NULL;
}

Example ClearScreen_Example = {
    "ClearScreen", Init, Update, Draw, Quit
};
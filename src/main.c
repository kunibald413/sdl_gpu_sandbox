#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "shaders.h"
#include "example_base.h"
#include "examples.h"

static Example *Examples[] = {
    &ClearScreen_Example,
    &FixedTriangle_Example,
    &BasicTriangleVertexBuffer_Example,
};
#define NUM_EXAMPLES SDL_arraysize(Examples)

#define WINDOW_TITLE  "Demo"
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define CLEAR_R 0.01f
#define CLEAR_G 0.01f
#define CLEAR_B 0.02f
#define CLEAR_A 1.0f


typedef struct AppState {
    Context Context;
    int CurrentExample;
} AppState;


SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("SDL version: %d", SDL_VERSION);

    SDL_GPUDevice *device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV |   // Vulkan    
        SDL_GPU_SHADERFORMAT_DXIL  |   // D3D12     
        SDL_GPU_SHADERFORMAT_MSL,      // Metal     
        true,                          // debug on  
        NULL                           // no backend preference 
    );
    if (!device) {
        SDL_Log("Failed to create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_Log("GPU backend: %s", SDL_GetGPUDeviceDriver(device));

    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE;
    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        window_flags
    );
    if (!window) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        SDL_DestroyGPUDevice(device);
        return SDL_APP_FAILURE;
    }

    // this binds the window's surface to our GPU device, creating the
    // swapchain (a ring of textures the OS composites to the screen)
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_DestroyGPUDevice(device);
        return SDL_APP_FAILURE;
    }

    if (SDL_WindowSupportsGPUSwapchainComposition(
            device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR)) {
        SDL_SetGPUSwapchainParameters(
            device, window,
            SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR,
            SDL_GPU_PRESENTMODE_VSYNC);
    }

    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        SDL_Log("Failed to allocate app state");
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyWindow(window);
        SDL_DestroyGPUDevice(device);
        return SDL_APP_FAILURE;
    }
    state->Context.Window = window;
    state->Context.Device = device;

    *appstate = state;

    state->CurrentExample = 2;

    Examples[state->CurrentExample]->Init(&state->Context);

    return SDL_APP_CONTINUE;
}

// called once per event
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    (void)appstate;

    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

// called once per frame
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *state = (AppState *)appstate;

    Examples[state->CurrentExample]->Draw(&state->Context);

    return SDL_APP_CONTINUE;
}

// Called once when the app is shutting down.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    AppState *state = (AppState *)appstate;
    if (state) {
        Context ctx = state->Context;
        SDL_ReleaseWindowFromGPUDevice(ctx.Device, ctx.Window);
        SDL_DestroyWindow(ctx.Window);
        SDL_DestroyGPUDevice(ctx.Device);
        SDL_free(state);
    }
}
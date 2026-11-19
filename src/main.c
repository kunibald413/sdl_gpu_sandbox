#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "shaders.h"

#define WINDOW_TITLE  "Demo"
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define CLEAR_R 0.01f
#define CLEAR_G 0.01f
#define CLEAR_B 0.02f
#define CLEAR_A 1.0f


typedef struct AppState {
    SDL_Window    *Window;
    SDL_GPUDevice *Device;

    // custom
    SDL_GPUGraphicsPipeline* CustomPipeline;
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

    SDL_WindowFlags window_flags = 0;
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
    state->Window = window;
    state->Device = device;

    *appstate = state;

    // custom
    SDL_GPUShader* frag_empty = LoadShader(device, "Empty.frag", 0,0,0,0);
    SDL_GPUShader* vert_no_input_triangle = LoadShader(device, "NoInputTriangle.vert", 0,0,0,0);
    SDL_GPUShader* frag_solid_color = LoadShader(device, "SolidColor.frag", 0,0,0,0);

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(device, window)
                }},
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = vert_no_input_triangle,
            .fragment_shader = frag_solid_color,
        };

	pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	state->CustomPipeline = SDL_CreateGPUGraphicsPipeline(device, &pipelineCreateInfo);

    SDL_ReleaseGPUShader(device, frag_empty);
	SDL_ReleaseGPUShader(device, vert_no_input_triangle);
	SDL_ReleaseGPUShader(device, frag_solid_color);
    // end custom

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

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(state->Device);
    if (!cmd) {
        SDL_Log("Failed to acquire command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain = NULL;
    if (!SDL_AcquireGPUSwapchainTexture(cmd, state->Window, &swapchain, NULL, NULL)) {
        SDL_Log("Failed to acquire swapchain: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchain) {
        SDL_GPUColorTargetInfo color_target = { 0 };
        color_target.texture     = swapchain;
        color_target.load_op     = SDL_GPU_LOADOP_CLEAR;
        color_target.store_op    = SDL_GPU_STOREOP_STORE;
        color_target.clear_color = (SDL_FColor){ CLEAR_R, CLEAR_G, CLEAR_B, CLEAR_A };

        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(
            cmd,
            &color_target, 1,   // one color target
            NULL                // no depth/stencil
        );

        // custom
        SDL_BindGPUGraphicsPipeline(pass, state->CustomPipeline);
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
        // end custom

        SDL_EndGPURenderPass(pass);
    }

    {
        SDL_SubmitGPUCommandBuffer(cmd);
    }

    return SDL_APP_CONTINUE;
}

// Called once when the app is shutting down.
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;

    AppState *state = (AppState *)appstate;
    if (state) {
        SDL_ReleaseWindowFromGPUDevice(state->Device, state->Window);
        SDL_DestroyWindow(state->Window);
        SDL_DestroyGPUDevice(state->Device);
        SDL_free(state);
    }
}
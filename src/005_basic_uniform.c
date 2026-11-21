
#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"


// aka Constant Buffer
typedef struct BasicUniform {
    SDL_GPUGraphicsPipeline* Pipeline;
    Uint64 ExampleStartTicks;
} BasicUniform;

// must match cbuffer in vert shader
typedef struct BasicUnifromData {
    float Time;
} BasicUniformData;


static int Init(Context *c) {
    BasicUniform* data = SDL_calloc(1, sizeof(BasicUniform));
    if (!data) return 1;

    SDL_GPUShader* vert = LoadShader(c->Device, "BasicUniform.vert", 0, 1, 0, 0);
    SDL_GPUShader* frag = LoadShader(c->Device, "SolidColor.frag", 0, 0, 0, 0);

    SDL_GPUGraphicsPipelineCreateInfo info = {0};
    info.vertex_shader = vert;
    info.fragment_shader = frag;
    info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    info.target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
        .num_color_targets = 1,
        .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
            .format = SDL_GetGPUSwapchainTextureFormat(c->Device, c->Window)
        }},
    };

    SDL_GPUGraphicsPipeline* pipe = SDL_CreateGPUGraphicsPipeline(c->Device, &info);

    SDL_ReleaseGPUShader(c->Device, vert);
	SDL_ReleaseGPUShader(c->Device, frag);
    
    if (!pipe) {
        SDL_Log("Failed to create pipeline %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    data->ExampleStartTicks = SDL_GetTicks();
    data->Pipeline = pipe;
    c->ExampleData = data;

    return 0;
}

static int Update(Context *c) {
    BasicUniform* data = (BasicUniform *)c->ExampleData;
    (void)data;
    return 0;
}

static int Draw(Context *c) {
    BasicUniform* data = (BasicUniform *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    Uint64 now_ms = SDL_GetTicks();
    float elapsed = (float)(now_ms - data->ExampleStartTicks) / 1000.0f;
    float rotation_speed = 1.0;
    BasicUniformData uniform_data = {.Time = elapsed * rotation_speed };
    Uint32 uniformt_slot = 0; // b0 in vert shader

    // must push before SDL_BeginGPURenderPass
    SDL_PushGPUVertexUniformData(cmd, uniformt_slot, &uniform_data, sizeof(uniform_data));

    SDL_GPUTexture *swapchain = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, NULL, NULL))
        return -1;

    if (swapchain) {
        SDL_GPUColorTargetInfo cti = {0};
        cti.texture     = swapchain;
        cti.clear_color = (SDL_FColor){0.0, 0.01, 0.01, 1.0};
        cti.load_op     = SDL_GPU_LOADOP_CLEAR;
        cti.store_op    = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass *p = SDL_BeginGPURenderPass(cmd, &cti, 1, NULL);

        SDL_BindGPUGraphicsPipeline(p, data->Pipeline);
        SDL_DrawGPUPrimitives(p, 3, 1, 0, 0);

        SDL_EndGPURenderPass(p);
    }
    SDL_SubmitGPUCommandBuffer(cmd);

    return 0;
}

static void Quit(Context *c) {
    BasicUniform* data = c->ExampleData;
    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;
    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicUniform_Example = {
    "BasicUniform", Init, Update, Draw, Quit
};
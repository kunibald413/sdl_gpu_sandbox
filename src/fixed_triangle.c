
#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

typedef struct FixedTriangleData {
    SDL_FColor ClearColor;
    SDL_GPUGraphicsPipeline* Pipeline;
} FixedTriangleData;

static int Init(Context *c) {
    FixedTriangleData* data = SDL_calloc(1, sizeof(FixedTriangleData));
    if (!data) return 1;
    
    data->ClearColor = (SDL_FColor){0.0, 0.01, 0.02, 1.0};

    SDL_GPUShader* vert = LoadShader(c->Device, "NoInputTriangle.vert", 0,0,0,0);
    SDL_GPUShader* frag = LoadShader(c->Device, "ColorPassthrough.frag", 0,0,0,0);

    SDL_GPUGraphicsPipelineCreateInfo pipelineCreateInfo = {
            .target_info = {
                .num_color_targets = 1,
                .color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
                    .format = SDL_GetGPUSwapchainTextureFormat(c->Device, c->Window)
                }},
            },
            .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
            .vertex_shader = vert,
            .fragment_shader = frag,
        };

	pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	data->Pipeline = SDL_CreateGPUGraphicsPipeline(c->Device, &pipelineCreateInfo);

	SDL_ReleaseGPUShader(c->Device, vert);
	SDL_ReleaseGPUShader(c->Device, frag);

    c->ExampleData = data;

    return 0;
}

static int Update(Context *c) {
    (void)c;
    return 0;
}

static int Draw(Context *c) {
    FixedTriangleData* data = (FixedTriangleData *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    SDL_GPUTexture *swapchain = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, NULL, NULL))
        return -1;

    if (swapchain) {
        SDL_GPUColorTargetInfo cti = {0};
        cti.texture     = swapchain;
        cti.clear_color = data->ClearColor;
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
    FixedTriangleData* data = (FixedTriangleData *)c->ExampleData;
    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;
    SDL_free(data);
    c->ExampleData = NULL;
}

Example FixedTriangle_Example = {
    "FixedTriangle", Init, Update, Draw, Quit
};
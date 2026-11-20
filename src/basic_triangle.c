#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

typedef struct BasicTriangle {
    SDL_FColor ClearColor;
    SDL_GPUGraphicsPipeline* Pipeline;
} BasicTriangle;


static int Init(Context *c) {
    BasicTriangle* data = (BasicTriangle*)SDL_calloc(1, sizeof(BasicTriangle));
    if (!data) return 1;

    SDL_GPUShader *vert = LoadShader(c->Device, "Triangle.vert", 0, 0, 0, 0);
    SDL_GPUShader *frag = LoadShader(c->Device, "SolidColor.frag", 0, 0, 0, 0);

    if (!vert || !frag) {
        SDL_Log("Failed to load shaders %s", __FILE__);
        return 1;
    }

    SDL_Log("Loaded shaders %s", __FILE__);

    SDL_GPUGraphicsPipelineCreateInfo pipe_info = {
        .target_info = {
            .num_color_targets = 1,
            .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
                .format = SDL_GetGPUSwapchainTextureFormat(c->Device, c->Window)
            }},
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_shader = vert,
        .fragment_shader = frag,
    };
    
    pipe_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    data->Pipeline = SDL_CreateGPUGraphicsPipeline(c->Device, &pipe_info);

    SDL_ReleaseGPUShader(c->Device, vert);
    SDL_ReleaseGPUShader(c->Device, frag);

    if (!data->Pipeline) {
        SDL_Log("Failed to create pipeline %s", __FILE__);
        return 1;
    }


    data->ClearColor = (SDL_FColor){0.1,0.03, 0.04, 1.0};
    c->ExampleData = data;
    return 0;
}

static int Update(Context *c) {

}

static int Draw(Context *c) {
    BasicTriangle* data = (BasicTriangle*)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);
    SDL_GPUTexture *swapchain = NULL;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, NULL, NULL)) {
        return 1;
    }

    if (swapchain) {
        SDL_GPUColorTargetInfo cinfo = {
            .texture = swapchain,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .clear_color = data->ClearColor
        };
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &cinfo, 1, NULL);

        SDL_BindGPUGraphicsPipeline(pass, data->Pipeline);
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd);
}

static void Quit(Context *c) {
    BasicTriangle* data = (BasicTriangle*)c->ExampleData;
    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;
    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicTriangle_Example = {
    "BasicTriangle", Init, Update, Draw, Quit
};
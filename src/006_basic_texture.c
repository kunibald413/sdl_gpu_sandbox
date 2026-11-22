
#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"
#include "img.h"

typedef struct BasicTexture {
    SDL_FColor ClearColor;
    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPUTexture* Texture;
} BasicTexture;

static int Init(Context *c) {
    BasicTexture* data = SDL_calloc(1, sizeof(BasicTexture));
    if (!data) return 1;
    
    data->ClearColor = (SDL_FColor) { 0.085, 0.17, 0.11, 1.0};

    // todo load right shaders in right way
    // setup texture
    // setup texture sampler
    // setup vertex and index buffers
    
    SDL_GPUShader* vert = LoadShader(c->Device, "NoInputQuad.vert", 0,0,0,0);
    SDL_GPUShader* frag = LoadShader(c->Device, "ColorPassthrough.frag", 0,0,0,0);
    
    const char* BasePath = SDL_GetBasePath();
    char fullPath[256];
    SDL_snprintf(fullPath, sizeof(fullPath), "%sassets/%s", BasePath, "space_ambient.png");
    SDL_GPUTexture *texture = LoadTexture(c->Device, fullPath);
    if (!texture) {
        SDL_Log("Failed to load texture buffer %s %s %s", SDL_GetError(), fullPath, __FILE__);
        return 1;
    }

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

	pipelineCreateInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;
	SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(c->Device, &pipelineCreateInfo);
    
	SDL_ReleaseGPUShader(c->Device, vert);
	SDL_ReleaseGPUShader(c->Device, frag);
    
    
	data->Pipeline = pipe;
    c->ExampleData = data;

    return 0;
}

static int Update(Context *c) {
    (void)c;
    return 0;
}

static int Draw(Context *c) {
    BasicTexture* data = (BasicTexture *)c->ExampleData;

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
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &cti, 1, NULL);

        SDL_BindGPUGraphicsPipeline(pass, data->Pipeline);
        // bind vertex buffer
        // bind index buffer
        // bind gpu texture
        // bind sampler
        SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 0, 0, 0);

        SDL_EndGPURenderPass(pass);
    }
    SDL_SubmitGPUCommandBuffer(cmd);

    return 0;
}

static void Quit(Context *c) {
    BasicTexture* data = (BasicTexture *)c->ExampleData;
    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    SDL_ReleaseGPUTexture(c->Device, data->Texture);
    data->Texture = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicTexture_Example = {
    "BasicTexture", Init, Update, Draw, Quit
};
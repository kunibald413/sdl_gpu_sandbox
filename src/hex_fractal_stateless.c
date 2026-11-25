#include <SDL3/SDL.h>
#include "example_base.h"

// must match [numthreads(8, 8, 1)] in compute shader
#define HEX_COMPUTE_THREADS 8
#define HEX_COMPUTE_MAX_VARIETY 9
#define HEX_COMPUTE_MAX_STEPS 14

#define HAX_FRACTAL_STATELESS_NAME "HexFractalStateless"

typedef struct HexFractalContext {
    SDL_GPUComputePipeline *Pipeline;
    SDL_GPUTexture *Texture;
    Uint32 TextureWidth;
    Uint32 TextureHeight;

    // Fractal parameters
    Uint32 Seed;
    Sint32 Variety;
    Sint32 Steps;
    float Zoom;
    float PanX;
    float PanY;
    bool Dragging;
} HexFractalContext;

// Matches cbuffer UniformBuffer : register(b0, space2) (32 bytes, 16-byte aligned)
typedef struct HexFractalConfig {
    Uint32 Seed;
    Sint32 Variety;
    Sint32 Steps;
    float Zoom;
    float PanX;
    float PanY;
    float Time;
    float _Pad;
} HexFractalConfig;

static int Init(Context *c) {
    HexFractalContext *data = SDL_calloc(1, sizeof(HexFractalContext));
    if (!data) return 1;

    // load shader
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(c->Device);

    char fullPath[256];
    const char *entrypoint;
    const char *BasePath = SDL_GetBasePath();
    const char *shaderFilename = "HexFractalStateless.comp";
    SDL_GPUShaderFormat format;

    if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.spv", BasePath, shaderFilename);
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.dxil", BasePath, shaderFilename);
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_DXIL;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_snprintf(fullPath, sizeof(fullPath), "%sshaders/compiled/%s.msl", BasePath, shaderFilename);
        entrypoint = "main0";
        format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        SDL_Log("No supported shader format");
        SDL_free(data);
        return 1;
    }

    size_t size;
    void *shader_data = SDL_LoadFile(fullPath, &size);
    if (!shader_data) {
        SDL_Log("Failed to load shader: %s, fp: %s", SDL_GetError(), fullPath);
        SDL_free(data);
        return 1;
    }

    // compute pipeline
    SDL_GPUComputePipelineCreateInfo cpci = {0};
    cpci.code = shader_data;
    cpci.code_size = size;
    cpci.entrypoint = entrypoint;
    cpci.format = format;
    cpci.num_readwrite_storage_textures = 1; // u0 in space1
    cpci.num_uniform_buffers = 1;            // b0 in space2
    cpci.threadcount_x = HEX_COMPUTE_THREADS;
    cpci.threadcount_y = HEX_COMPUTE_THREADS;
    cpci.threadcount_z = 1;

    SDL_GPUComputePipeline *pipeline = SDL_CreateGPUComputePipeline(c->Device, &cpci);
    SDL_free(shader_data);
    if (!pipeline) {
        SDL_Log("Failed to create compute pipeline: %s", SDL_GetError());
        SDL_free(data);
        return 1;
    }

    data->Pipeline = pipeline;
    data->Texture = NULL;
    data->TextureWidth = 0;
    data->TextureHeight = 0;

    // init state
    data->Seed = 132;
    data->Variety = 4;
    data->Steps = 9;
    data->Zoom = 3.0f;
    data->PanX = 0.0f;
    data->PanY = 0.0f;
    data->Dragging = false;

    c->ExampleData = data;

    SDL_Log("Keys: [S/X] Seed, [Up/Down] Steps, [V] Variety | Mouse: Drag to pan, Scroll to zoom");
    return 0;
}

static void OnAppEvent(Context *c, SDL_Event *event) {
    HexFractalContext *ctx = (HexFractalContext *)c->ExampleData;

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_S) ctx->Seed++;
        if (event->key.key == SDLK_X && ctx->Seed > 1) ctx->Seed--;
        if (event->key.key == SDLK_UP && ctx->Steps < HEX_COMPUTE_MAX_STEPS) ctx->Steps++;
        if (event->key.key == SDLK_DOWN && ctx->Steps > 1) ctx->Steps--;
        if (event->key.key == SDLK_V) {
            ctx->Variety += 1;
            if (ctx->Variety > HEX_COMPUTE_MAX_VARIETY) ctx->Variety = 2;
        } 
    }

    // click and drag to pan around the infinite hex map
    if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN && event->button.button == SDL_BUTTON_LEFT) {
        ctx->Dragging = true;
    }
    if (event->type == SDL_EVENT_MOUSE_BUTTON_UP && event->button.button == SDL_BUTTON_LEFT) {
        ctx->Dragging = false;
    }
    if (event->type == SDL_EVENT_MOUSE_MOTION && ctx->Dragging && ctx->TextureWidth > 0 && ctx->TextureHeight > 0) {
        ctx->PanX -= (event->motion.xrel / (float)ctx->TextureWidth) * ctx->Zoom;
        ctx->PanY -= (event->motion.yrel / (float)ctx->TextureHeight) * ctx->Zoom;
    }

    // zoom in/out
    if (event->type == SDL_EVENT_MOUSE_WHEEL) {
        float zoomFactor = (event->wheel.y > 0) ? 0.85f : 1.15f;
        ctx->Zoom *= zoomFactor;
        if (ctx->Zoom < 0.02f) ctx->Zoom = 0.02f;
        if (ctx->Zoom > 50.0f) ctx->Zoom = 50.0f;
    }
}

static int Update(Context *c) {
    (void)c;
    return 0;
}

static int Draw(Context *c) {
    HexFractalContext *data = (HexFractalContext *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    SDL_GPUTexture *swapchain = NULL;
    Uint32 width;
    Uint32 height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, &width, &height)) {
        return -1;
    }

    if (swapchain) {
        if (data->TextureWidth != width || data->TextureHeight != height) {
            if (data->Texture) SDL_ReleaseGPUTexture(c->Device, data->Texture);
            SDL_GPUTextureCreateInfo tci = {0};
            tci.usage = SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE | SDL_GPU_TEXTUREUSAGE_SAMPLER;
            tci.type = SDL_GPU_TEXTURETYPE_2D;
            tci.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
            tci.width = width;
            tci.height = height;
            tci.num_levels = 1;
            tci.layer_count_or_depth = 1;

            data->Texture = SDL_CreateGPUTexture(c->Device, &tci);
            if (!data->Texture) {
                SDL_Log("Failed to create texture: %s", SDL_GetError());
                SDL_SubmitGPUCommandBuffer(cmd);
                return 1;
            }
            data->TextureWidth = width;
            data->TextureHeight = height;
        }

        // register(b0, space2)
        HexFractalConfig uniform_data = {
            .Seed = data->Seed,
            .Variety = data->Variety,
            .Steps = data->Steps,
            .Zoom = data->Zoom,
            .PanX = data->PanX,
            .PanY = data->PanY,
            .Time = SDL_GetTicks() / 1000.0f,
            ._Pad = 0.0f
        };
        SDL_PushGPUComputeUniformData(cmd, 0, &uniform_data, sizeof(uniform_data));

        // run compute
        SDL_GPUStorageTextureReadWriteBinding stb = {0};
        stb.texture = data->Texture;
        SDL_GPUComputePass *compute_pass = SDL_BeginGPUComputePass(cmd, &stb, 1, NULL, 0);
        if (!compute_pass) {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(cmd);
            return 1;
        }

        const int n_threads = HEX_COMPUTE_THREADS;
        SDL_BindGPUComputePipeline(compute_pass, data->Pipeline);
        SDL_DispatchGPUCompute(
            compute_pass,
            (width + n_threads - 1) / n_threads,
            (height + n_threads - 1) / n_threads,
            1
        );
        SDL_EndGPUComputePass(compute_pass);

        // AFTER pass
        SDL_GPUBlitInfo blit = {0};
        blit.source.texture = data->Texture;
        blit.source.w = width;
        blit.source.h = height;
        blit.destination.texture = swapchain;
        blit.destination.w = width;
        blit.destination.h = height;
        SDL_BlitGPUTexture(cmd, &blit);
    }

    SDL_SubmitGPUCommandBuffer(cmd);

    char title[128];
    SDL_snprintf(title, sizeof(title), "%s variety: %d steps: %d seed: %d", 
        HAX_FRACTAL_STATELESS_NAME, data->Variety, data->Steps, data->Seed);
    SDL_SetWindowTitle(c->Window, title);
    return 0;
}

static void Quit(Context *c) {
    HexFractalContext *data = c->ExampleData;

    if (data->Pipeline) SDL_ReleaseGPUComputePipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    if (data->Texture) SDL_ReleaseGPUTexture(c->Device, data->Texture);
    data->Texture = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example HexFractalStateless_Example = {
    HAX_FRACTAL_STATELESS_NAME, Init, Update, Draw, Quit, OnAppEvent
};

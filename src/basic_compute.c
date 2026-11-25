
#include <SDL3/SDL.h>
#include "example_base.h"

// as in shader
#define BASIC_COMPUTE_THREADS 8

typedef struct BasicComputeContext {
    SDL_FColor Color;
    SDL_GPUComputePipeline *Pipeline;
    SDL_GPUTexture *Texture;
    Uint32 TextureWidth;
    Uint32 TextureHeight;
    float Time;
    float MouseX;
    float MouseY;
} BasicComputeContext;

typedef struct BasicComputeConfig {
    float MouseX;
    float MouseY;
} BasicComputeConfig;

static int Init(Context *c) {
    BasicComputeContext* data = SDL_calloc(1, sizeof(BasicComputeContext));
    if (!data) return 1;

    // shader
    SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(c->Device);

	char fullPath[256];
    const char* entrypoint;
    const char* BasePath = SDL_GetBasePath();
    const char* shaderFilename = "BasicCompute.comp";
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
        return 1;
    }
    size_t size;
    void* shader_data = SDL_LoadFile(fullPath, &size);
    if (!shader_data) {
        SDL_Log("Failed to load shader: %s, fp: %s", SDL_GetError(), fullPath);
        return 1;
    }

    // pipeline
    SDL_GPUComputePipelineCreateInfo cpci = {0};
    cpci.code = shader_data;
    cpci.code_size = size;
    cpci.entrypoint = entrypoint;
    cpci.format = format;
    cpci.num_readwrite_storage_textures = 1;
    cpci.num_uniform_buffers = 1;
    cpci.threadcount_x = BASIC_COMPUTE_THREADS;
    cpci.threadcount_y = BASIC_COMPUTE_THREADS;
    cpci.threadcount_z = 1;
    SDL_GPUComputePipeline* pipeline = SDL_CreateGPUComputePipeline(c->Device, &cpci);
    SDL_free(shader_data);
    if (!pipeline) {
        SDL_Log("Failed to create compute pipeline: %s", SDL_GetError());
        return 1;
    }

    // wire
    data->Color = (SDL_FColor) {0.001, 0.00027, 0.02, 1.0};
    
    data->Pipeline = pipeline;
    data->Texture = NULL;
    data->TextureWidth = 0;
    data->TextureHeight = 0;
    data->Time = 1.0;
    data->MouseX = 0.5;
    data->MouseY = 0.5;

    c->ExampleData = data;

    return 0;
}

static void OnAppEvent(Context *c, SDL_Event *event) {
    BasicComputeContext* ctx = (BasicComputeContext *)c->ExampleData;

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_DOWN) {
           ctx->Time -= 0.05;
           if (ctx->Time < 0.0) ctx->Time = 0.0;
        }
        if (event->key.key == SDLK_UP) {
            ctx->Time += 0.05;
            if (ctx->Time > 1.0) ctx->Time = 1.0;
        }
    }

    if (event->type == SDL_EVENT_MOUSE_MOTION
        && ctx->TextureWidth > 0 
        && ctx->TextureHeight > 0) {
        ctx->MouseX = event->motion.x / ctx->TextureWidth;
        ctx->MouseY = event->motion.y / ctx->TextureHeight;
    }
}

static int Update(Context *c) {
    (void)c;
    return 0;
}

static int Draw(Context *c) {
    BasicComputeContext* data = (BasicComputeContext *)c->ExampleData;

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

        //SDL_Log("w %d h %d tw %d th %d", width, height, data->TextureWidth, data->TextureHeight);
        
        BasicComputeConfig uniform_data = {.MouseX = data->MouseX, .MouseY = data->MouseY};
        Uint32 uniformt_slot = 0; // b0 in vert shader
        SDL_PushGPUComputeUniformData(cmd, uniformt_slot, &uniform_data, sizeof(uniform_data));


        SDL_GPUStorageTextureReadWriteBinding stb = {0};
        stb.texture = data->Texture;
        SDL_GPUComputePass* compute_pass = SDL_BeginGPUComputePass(cmd, &stb, 1, NULL, 0);
        if (!compute_pass) {
            SDL_Log("Failed to begin compute pass: %s", SDL_GetError());
            SDL_SubmitGPUCommandBuffer(cmd);
            return 1;
        }

        const int n_threads = BASIC_COMPUTE_THREADS;
        SDL_BindGPUComputePipeline(compute_pass, data->Pipeline);
        SDL_DispatchGPUCompute(compute_pass, 
            (width + n_threads - 1) / n_threads, 
            (height + n_threads - 1) / n_threads, 
            1
        );
        SDL_EndGPUComputePass(compute_pass);

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

    return 0;
}

static void Quit(Context *c) {
    BasicComputeContext* data = c->ExampleData;

    if (data->Pipeline) SDL_ReleaseGPUComputePipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    if (data->Texture) SDL_ReleaseGPUTexture(c->Device, data->Texture);
    data->Texture = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicCompute_Example = {
    "BasicCompute", Init, Update, Draw, Quit, OnAppEvent
};
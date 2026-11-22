
#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"
#include "img.h"

typedef struct BasicTexture {
    SDL_FColor ClearColor;
    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPUTexture* Texture;
    SDL_GPUSampler* Sampler;
    SDL_GPUBuffer* VertexBuffer;
    SDL_GPUBuffer* IndexBuffer;
} BasicTexture;

typedef struct BasicTextureVertex {
    float x,y; // Position
    float u,v;
} BasicTextureVertex;

// key points:
// create and upload buffers SDL_UploadToGPUBuffer
// upload gpu texture SDL_UploadToGPUTexture
// create sampler for texture SDL_CreateGPUSampler
// describe and create pipeline SDL_CreateGPUGraphicsPipeline

// bind stuff in render pass SDL_Bind*

// forget ANYTHING and get segfaults

static int Init(Context *c) {
    BasicTexture* data = SDL_calloc(1, sizeof(BasicTexture));
    if (!data) return 1;
    
    data->ClearColor = (SDL_FColor) { 0.085, 0.17, 0.11, 1.0};


    
    SDL_GPUShader* vert = LoadShader(c->Device, "BasicTexture.vert", 0,0,0,0);
    SDL_GPUShader* frag = LoadShader(c->Device, "BasicTexture.frag", 1,0,0,0);
    
    BasicTextureVertex vertices[4] = {
        {-0.75,  0.75, 0, 0}, // TL
        { 0.75,  0.75, 1, 0}, // TR
        { 0.75, -0.75, 1, 1}, // BR
        {-0.75, -0.75, 0, 1}, // BL
    };

    Uint16 indices[6] = {
        0,1,2,
        0,3,2
    };

    // vert and indx buffer uploads
    SDL_GPUBufferCreateInfo buffer_info = {0};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = sizeof(vertices);
    SDL_GPUBuffer* vertex_buffer = SDL_CreateGPUBuffer(c->Device, &buffer_info);
    
    SDL_GPUBufferCreateInfo buffer_info_index = {0};
    buffer_info_index.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    buffer_info_index.size = sizeof(indices);
    SDL_GPUBuffer* index_buffer = SDL_CreateGPUBuffer(c->Device, &buffer_info_index);

    SDL_GPUCommandBuffer * cmd = SDL_AcquireGPUCommandBuffer(c->Device);
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferCreateInfo transfer_buff_info = {0};
    transfer_buff_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buff_info.size = sizeof(vertices) + sizeof(indices);
    SDL_GPUTransferBuffer* transfer_buffer = SDL_CreateGPUTransferBuffer(c->Device, &transfer_buff_info);

    char* map = SDL_MapGPUTransferBuffer(c->Device, transfer_buffer, false);

    SDL_memcpy(map, &vertices, sizeof(vertices));
    SDL_memcpy(map + sizeof(vertices), &indices, sizeof(indices));

    SDL_UnmapGPUTransferBuffer(c->Device, transfer_buffer);

    SDL_GPUTransferBufferLocation src = {.offset = 0, .transfer_buffer = transfer_buffer};
    SDL_GPUBufferRegion dst = {.buffer = vertex_buffer, .offset = 0, .size = sizeof(vertices)};
    SDL_UploadToGPUBuffer(pass, &src, &dst, false);

    SDL_GPUTransferBufferLocation src_idx = {.offset = sizeof(vertices), .transfer_buffer = transfer_buffer};
    SDL_GPUBufferRegion dst_idx = {.buffer = index_buffer, .offset = 0, .size = sizeof(indices)};
    SDL_UploadToGPUBuffer(pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(pass);
	SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(c->Device, transfer_buffer);
    // end buffer upload

    // gpu texture
    const char* BasePath = SDL_GetBasePath();
    char fullPath[256];
    SDL_snprintf(fullPath, sizeof(fullPath), "%sassets/%s", BasePath, "space_ambient.png");
    SDL_GPUTexture *texture = LoadTexture(c->Device, fullPath);
    if (!texture) {
        SDL_Log("Failed to load texture buffer %s %s %s", SDL_GetError(), fullPath, __FILE__);
        return 1;
    }

    // sampler
    SDL_GPUSamplerCreateInfo sampler_info = {0};
    sampler_info.min_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mag_filter = SDL_GPU_FILTER_LINEAR;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    SDL_GPUSampler* sampler = SDL_CreateGPUSampler(c->Device, &sampler_info);
    if (!sampler) {
        SDL_Log("Failed to create sampler %s %s", SDL_GetError(), __FILE__);
        return 1;
    }
    // end texture and sampler


    // pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipe_info = {
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

    pipe_info.vertex_input_state = (SDL_GPUVertexInputState) {
        // describe the buffer
        .num_vertex_buffers = 1,
        .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
            .slot = 0,
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0,
            .pitch = sizeof(BasicTextureVertex)
        }},

        // describe vertex attributes
        .num_vertex_attributes = 2, // pos and uv
        .vertex_attributes = (SDL_GPUVertexAttribute[]) {
            {.buffer_slot = 0, .location = 0, .offset = offsetof(BasicTextureVertex, x), .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2},
            {.buffer_slot = 0, .location = 1, .offset = offsetof(BasicTextureVertex, u), .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2},
        }
    };

	pipe_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
	SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(c->Device, &pipe_info);
    
	SDL_ReleaseGPUShader(c->Device, vert);
	SDL_ReleaseGPUShader(c->Device, frag);
    
    // wire
	data->Pipeline = pipe;
    data->Sampler = sampler;
    data->VertexBuffer = vertex_buffer;
    data->IndexBuffer = index_buffer;
    data->Texture = texture;
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

        // wire texture rending 
        SDL_BindGPUGraphicsPipeline(pass, data->Pipeline);
        SDL_BindGPUVertexBuffers(pass, 0, &(SDL_GPUBufferBinding){.offset=0, .buffer = data->VertexBuffer}, 1);
        SDL_BindGPUIndexBuffer(pass, &(SDL_GPUBufferBinding){.offset=0, .buffer = data->IndexBuffer}, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_BindGPUFragmentSamplers(pass, 0, &(SDL_GPUTextureSamplerBinding) {.sampler = data->Sampler, .texture = data->Texture}, 1);
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
    
    SDL_ReleaseGPUSampler(c->Device, data->Sampler);
    data->Sampler = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->VertexBuffer);
    data->VertexBuffer = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->IndexBuffer);
    data->IndexBuffer = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicTexture_Example = {
    "BasicTexture", Init, Update, Draw, Quit
};
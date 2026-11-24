
#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

// vert buffer
// index buffer
// mat4 uniform
// vertex: position, color (3xfloat, 3xfloat)
// pipeline
// wireframe

typedef struct Basic3DPyramidContext {
    SDL_GPUGraphicsPipeline *Pipeline;
    SDL_GPUBuffer *VertexBuffer;
    SDL_GPUBuffer *IndexBuffer;
} Basic3DPyramidContext;


SDL_GPUVertexElementFormat attr_format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
typedef struct Basic3DPyramidVert {
    float x,y,z; // pos
    float r,g,b; // color
} Basic3DPyramidVert;


static int Init(Context *c) {

    Basic3DPyramidContext *ctx = (Basic3DPyramidContext *)SDL_calloc(1, sizeof(Basic3DPyramidContext));

    if (!ctx) {SDL_Log("Failed to alloc? %s", SDL_GetError()); return 1; }
    
    // shaders
    SDL_GPUDevice *device = c->Device;
    SDL_Window *window = c->Window;
    
    SDL_GPUShader *vert = LoadShader(device, "Basic3DPyramid.vert", 0, 1, 0, 0);
    if (!vert) {SDL_Log("Failed create vert shader %s", SDL_GetError()); return 1; }

    SDL_GPUShader *frag = LoadShader(device, "ColorPassthrough.frag", 0, 0, 0, 0);
    if (!frag) {SDL_Log("Failed create frag shader %s", SDL_GetError()); return 1; }

    // pipeline
    SDL_GPUGraphicsPipelineCreateInfo pi = {0};
    pi.vertex_shader = vert;
    pi.fragment_shader = frag;
    pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pi.target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
        .num_color_targets = 1,
        .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
            .format = SDL_GetGPUSwapchainTextureFormat(device, window)
        }}, 
    };
    pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_LINE;

    const Uint16 BUFF_SLOT = 0;
    pi.vertex_input_state = (SDL_GPUVertexInputState) {
        .num_vertex_buffers = 1,
        .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {{
            .slot = BUFF_SLOT,
            .pitch = sizeof(Basic3DPyramidVert),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        }},
        .num_vertex_attributes = 2,
        .vertex_attributes = (SDL_GPUVertexAttribute[]) {
            // pos
            {.location = 0, .offset = offsetof(Basic3DPyramidVert, x), .buffer_slot = BUFF_SLOT, .format = attr_format},
            // color
            {.location = 1, .offset = offsetof(Basic3DPyramidVert, r), .buffer_slot = BUFF_SLOT, .format = attr_format},
        }
    };

    SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(device, &pi);

    SDL_ReleaseGPUShader(device, frag);
    SDL_ReleaseGPUShader(device, vert);


    if (!pipe) {
        SDL_Log("Failed to create pipeline %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    // buffers: vertex, index

    Basic3DPyramidVert verticies[4] = {
        {  0.0,  0.5,   0.0,  0.5, 0.5, 0.0}, // TOP
        { -0.5, -0.5,   0.5,  0.0, 1.0, 1.0}, // BOT LEFT
        {  0.5, -0.5,   0.5,  1.0, 0.0, 1.0}, // BOT RIGHT
        {  0.0,  0.0,  -0.5,  1.0, 1.0, 0.0}, // BACK
    };

    // 4 faces
    int indicies[4*3] = {
        0,1,2, // front
        0,2,3, // right
        0,1,3, // left
        1,2,3 // bot
    };

    // DEFINE BUFFERS AND TRANSFER, LORD JESUS HELP ME
    SDL_GPUBuffer *vert_buff = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(verticies),
        .props = 0
    });
    if (!vert_buff) {SDL_Log("Failed to create vert buff %s", SDL_GetError()); return 1; }
    SDL_GPUBuffer *index_buff = SDL_CreateGPUBuffer(device, &(SDL_GPUBufferCreateInfo) {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = sizeof(indicies),
        .props = 0
    });
    if (!index_buff) {SDL_Log("Failed to create idx buff %s", SDL_GetError()); return 1; }
    
    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {SDL_Log("Failed to get cmd buff %s", SDL_GetError()); return 1; }
    SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(cmd);
    if (!pass) {SDL_Log("Failed to get copy pass %s", SDL_GetError()); return 1; }
    
    SDL_GPUTransferBuffer *transfer_buff = SDL_CreateGPUTransferBuffer(device, &(SDL_GPUTransferBufferCreateInfo){
        .size = sizeof(verticies) + sizeof(indicies),
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .props = 0
    });
    if (!transfer_buff) {SDL_Log("Failed to create transfer buff %s", SDL_GetError()); return 1; }

    char* mapped = SDL_MapGPUTransferBuffer(device, transfer_buff, false);
    SDL_memcpy(mapped, verticies, sizeof(verticies));
    SDL_memcpy(mapped + sizeof(verticies), indicies, sizeof(indicies));
    SDL_UnmapGPUTransferBuffer(device, transfer_buff);

    // upload to vertex buffer
    SDL_GPUTransferBufferLocation src = {.offset = 0, .transfer_buffer = transfer_buff};
    SDL_GPUBufferRegion dst = {.buffer = vert_buff, .offset = 0, .size = sizeof(verticies) };
    SDL_UploadToGPUBuffer(pass, &src, &dst, false);

    // upload to idx buffer
    SDL_GPUTransferBufferLocation src_idx = {.offset = sizeof(verticies), .transfer_buffer = transfer_buff};
    SDL_GPUBufferRegion dst_idx = {.buffer = index_buff, .offset = 0, .size = sizeof(indicies)};
    SDL_UploadToGPUBuffer(pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buff);
    // END TRANSFER

    ctx->Pipeline = pipe;
    ctx->VertexBuffer = vert_buff;
    ctx->IndexBuffer = index_buff;
    c->ExampleData = ctx;

    return 0;
}

static int Update(Context *c) {
    return 0;
}

static int Draw(Context *c) {

    Basic3DPyramidContext* ctx = (Basic3DPyramidContext *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    // push before starting render pass, row major
    float matrix[4*4] = {
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };
    SDL_PushGPUVertexUniformData(cmd, 0, &matrix, sizeof(matrix));

    SDL_GPUTexture *swapchain = NULL;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, NULL, NULL))
        return -1;

    if (swapchain) {
        SDL_GPUColorTargetInfo cti = {0};
        cti.texture     = swapchain;
        cti.clear_color = (SDL_FColor) {0.01,0.01,0.01, 0.5};
        cti.load_op     = SDL_GPU_LOADOP_CLEAR;
        cti.store_op    = SDL_GPU_STOREOP_STORE;
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &cti, 1, NULL);

        SDL_BindGPUGraphicsPipeline(pass, ctx->Pipeline);

        SDL_GPUBufferBinding buff_bind = {0};
        buff_bind.buffer = ctx->VertexBuffer;
        buff_bind.offset = 0;
        SDL_BindGPUVertexBuffers(pass, 0, &buff_bind, 1);

        SDL_GPUBufferBinding buff_bind_idx = {0};
        buff_bind_idx.buffer = ctx->IndexBuffer;
        buff_bind_idx.offset = 0;
        SDL_BindGPUIndexBuffer(pass, &buff_bind_idx, SDL_GPU_INDEXELEMENTSIZE_32BIT);

        SDL_DrawGPUIndexedPrimitives(pass, 12, 1, 0, 0, 0);

        SDL_EndGPURenderPass(pass);
    }
    SDL_SubmitGPUCommandBuffer(cmd);

    return 0;
}

void Quit(Context *c) {
    Basic3DPyramidContext* data = (Basic3DPyramidContext*)c->ExampleData;

    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->VertexBuffer);
    data->VertexBuffer = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->IndexBuffer);
    data->IndexBuffer = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example Basic3DPyramid_Example  = {
    "Basic3DPyramid", Init, Update, Draw, Quit
};
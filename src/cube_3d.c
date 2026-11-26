

#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

typedef struct Cube3DContext {
    SDL_GPUGraphicsPipeline *Pipeline;
    SDL_GPUBuffer *VertexBuffer;
    SDL_GPUBuffer *IndexBuffer;
} Cube3DContext;

typedef struct Cube3DUniform {
    float MVP[4*4]; // row major
} Cube3DUniform;

typedef struct Cube3DVert {
    float x,y,z;
    float r,g,b;
} Cube3DVert;

// index buffer
// vertex buffer
// graphics pipe with depth
// calc mvp
// push uniforms
// bind vert and index buffers
// draw indexed

static int Init(Context *c) {
    (void)c;

    Cube3DContext *data = (Cube3DContext *)SDL_calloc(1, sizeof(Cube3DContext));

    SDL_GPUShader *vert = LoadShader(c->Device, "Cube3D.vert", 0, 1, 0, 0);
    if (!vert) {SDL_Log("Failed to load vert shader %s", SDL_GetError()); return 1;}

    SDL_GPUShader *frag = LoadShader(c->Device, "ColorPassthrough.frag", 0, 0, 0, 0);
    if (!frag) {SDL_Log("Failed to load frag shader %s", SDL_GetError()); return 1;}

    // VERT & INDEX BUFFERS
    const float CUBE_HALF = 0.5;
    Cube3DVert verticies[4*6] = {
        // front face
        {-CUBE_HALF,  CUBE_HALF,  CUBE_HALF, 0.5, 0.5, 0.0}, // 0
        { CUBE_HALF,  CUBE_HALF,  CUBE_HALF, 0.5, 0.5, 0.0},
        { CUBE_HALF, -CUBE_HALF,  CUBE_HALF, 0.5, 0.5, 0.0},
        {-CUBE_HALF, -CUBE_HALF,  CUBE_HALF, 0.5, 0.5, 0.0},

        // back 
        {-CUBE_HALF,  CUBE_HALF, -CUBE_HALF, 0.0, 0.5, 0.0}, // 4
        { CUBE_HALF,  CUBE_HALF, -CUBE_HALF, 0.0, 0.5, 0.0},
        { CUBE_HALF, -CUBE_HALF, -CUBE_HALF, 0.0, 0.5, 0.0},
        {-CUBE_HALF, -CUBE_HALF, -CUBE_HALF, 0.0, 0.5, 0.0},

        // top
        {-CUBE_HALF,  CUBE_HALF,  CUBE_HALF, 0.5, 0.0, 0.0}, // 8
        {-CUBE_HALF,  CUBE_HALF, -CUBE_HALF, 0.5, 0.0, 0.0},
        { CUBE_HALF,  CUBE_HALF, -CUBE_HALF, 0.5, 0.0, 0.0},
        { CUBE_HALF,  CUBE_HALF,  CUBE_HALF, 0.5, 0.0, 0.0},

        // bottom
        {-CUBE_HALF, -CUBE_HALF,  CUBE_HALF, 0.0, 0.0, 0.5}, // 12
        {-CUBE_HALF, -CUBE_HALF, -CUBE_HALF, 0.0, 0.0, 0.5},
        { CUBE_HALF, -CUBE_HALF, -CUBE_HALF, 0.0, 0.0, 0.5},
        { CUBE_HALF, -CUBE_HALF,  CUBE_HALF, 0.0, 0.0, 0.5},

        // left
        {-CUBE_HALF,  CUBE_HALF,  CUBE_HALF, 0.0, 0.5, 0.5}, // 16
        {-CUBE_HALF, -CUBE_HALF,  CUBE_HALF, 0.0, 0.5, 0.5},
        {-CUBE_HALF, -CUBE_HALF, -CUBE_HALF, 0.0, 0.5, 0.5},
        {-CUBE_HALF,  CUBE_HALF, -CUBE_HALF, 0.0, 0.5, 0.5},

        // right
        {CUBE_HALF,  CUBE_HALF,  CUBE_HALF, 0.5, 0.0, 0.5}, // 20
        {CUBE_HALF, -CUBE_HALF,  CUBE_HALF, 0.5, 0.0, 0.5},
        {CUBE_HALF, -CUBE_HALF, -CUBE_HALF, 0.5, 0.0, 0.5},
        {CUBE_HALF,  CUBE_HALF, -CUBE_HALF, 0.5, 0.0, 0.5},
    };

    // 6 faces, each 2 triangles, each 3 corners
    Uint32 indicies[6 * 2 * 3] = {
         0,  1,  2,  2,  3,  0, // front
         4,  5,  6,  6,  7,  4, // back
         8,  9, 10, 10, 11,  8, // top
        12, 13, 14, 14, 15, 12, // bottom
        16, 17, 18, 18, 19, 16, // left
        20, 21, 22, 22, 23, 20, // right
    };

    // vert + idx buffers
    SDL_GPUBufferCreateInfo vert_buff_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(verticies)
    };
    SDL_GPUBuffer *vert_buffer = SDL_CreateGPUBuffer(c->Device, &vert_buff_info);
    if (!vert_buffer) {SDL_Log("Failed to create vert buffer %s", SDL_GetError()); return 1;}
    
    SDL_GPUBufferCreateInfo idx_buff_info = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = sizeof(indicies)
    };
    SDL_GPUBuffer *idx_buffer = SDL_CreateGPUBuffer(c->Device, &idx_buff_info);
    if (!idx_buffer) {SDL_Log("Failed to create idx buffer %s", SDL_GetError()); return 1;}

    // transfer buffer
    SDL_GPUTransferBufferCreateInfo tfbi = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = sizeof(verticies) + sizeof(indicies)
    };
    SDL_GPUTransferBuffer *transfer_buff = SDL_CreateGPUTransferBuffer(c->Device, &tfbi);
    if (!transfer_buff) {SDL_Log("Failed to create transfer buffer %s", SDL_GetError()); return 1;}

    SDL_Log("SDL_MapGPUTransferBuffer");
    char *transfer_map = SDL_MapGPUTransferBuffer(c->Device, transfer_buff, false);
        SDL_memcpy(transfer_map, verticies, sizeof(verticies));
        SDL_memcpy(transfer_map + sizeof(verticies), indicies, sizeof(indicies));
    SDL_UnmapGPUTransferBuffer(c->Device, transfer_buff);
    
    // upload
    SDL_Log("SDL_AcquireGPUCommandBuffer");
    SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(c->Device);
    SDL_Log("SDL_BeginGPUCopyPass");
	SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd);
    
    SDL_Log("SDL_UploadToGPUBuffer vert");
    SDL_UploadToGPUBuffer(copy_pass, 
        &(SDL_GPUTransferBufferLocation) {.offset= 0, .transfer_buffer = transfer_buff}, // from
        &(SDL_GPUBufferRegion) {.offset = 0, .buffer = vert_buffer, .size = sizeof(verticies)},   // to
        false
    );
    SDL_Log("SDL_UploadToGPUBuffer idx");
    SDL_UploadToGPUBuffer(copy_pass, 
        &(SDL_GPUTransferBufferLocation) {.offset = sizeof(verticies), .transfer_buffer = transfer_buff}, // from
        &(SDL_GPUBufferRegion) {.offset = 0, .buffer = idx_buffer, .size = sizeof(indicies)},                     // to
        false
    );

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(upload_cmd);

    SDL_ReleaseGPUTransferBuffer(c->Device, transfer_buff);
    

    // PIPELINE
    SDL_GPUGraphicsPipelineCreateInfo pi = {0};

    pi.vertex_shader = vert;
    pi.fragment_shader = frag;
    pi.primitive_type = SDL_GPU_PRIMITIVETYPE_LINELIST;
    pi.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;


    // vert input 
    SDL_GPUVertexInputState vert_inp = {0};
    // vert input buffer desc
    SDL_GPUVertexBufferDescription vert_inp_buff_desc = {0};
    vert_inp_buff_desc.slot = 0;
    vert_inp_buff_desc.pitch = sizeof(Cube3DVert);
    vert_inp_buff_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vert_inp_buff_desc.instance_step_rate = 0;

    vert_inp.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {vert_inp_buff_desc};
    vert_inp.num_vertex_buffers = 1;

    // vert input attributes
    SDL_GPUVertexAttribute vert_attributes[2] = {
        // pos
        {.location = 0, .offset = offsetof(Cube3DVert, x), .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .buffer_slot = 0},
        // color
        {.location = 1, .offset = offsetof(Cube3DVert, r), .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, .buffer_slot = 0},
    };
    vert_inp.vertex_attributes = vert_attributes;
    vert_inp.num_vertex_attributes = 2;
    pi.vertex_input_state = vert_inp;

    // rasterizer
    pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;

    // target
    SDL_GPUGraphicsPipelineTargetInfo target_info = {0};
    SDL_GPUColorTargetDescription target_desc[1] = {
        {.format = SDL_GetGPUSwapchainTextureFormat(c->Device, c->Window)}
    };
    target_info.color_target_descriptions = target_desc;
    target_info.num_color_targets = 1;
    pi.target_info = target_info;

    // pi.rasterizer_state.enable_depth_clip = true;


    SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(c->Device, &pi);

    SDL_ReleaseGPUShader(c->Device, frag);
    SDL_ReleaseGPUShader(c->Device, vert);

    if (!pipe) {SDL_Log("Failed create pipeline %s", SDL_GetError()); return 1;}

    // wire

    data->Pipeline = pipe;
    data->VertexBuffer = vert_buffer;
    data->IndexBuffer = idx_buffer;

    c->ExampleData = data;

    return 0;
}

static int Update(Context *c) {
    (void)c;
    return 0;
}

static int Draw(Context *c) {
    (void)c;

    Cube3DContext *ctx = (Cube3DContext *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    SDL_GPUTexture *swap_tex;
    Uint32 width;
    Uint32 height;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swap_tex,&width, &height)){
        return -1;
    }

    if (swap_tex) {
        SDL_GPUColorTargetInfo ct = {
            .clear_color = (SDL_FColor) {0.00, 0.0, 0.00, 1.0},
            .texture = swap_tex,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE
        };
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &ct, 1, NULL);

        float t = (float)SDL_GetTicks()  / 1000.0;
        float s = SDL_sinf(t);
        float c = SDL_cosf(t);

        float mat_identity[4*4] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0,  0.0f,
            0.0f, 0.0,  1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };


        SDL_BindGPUGraphicsPipeline(pass, ctx->Pipeline);
        SDL_PushGPUVertexUniformData(cmd, 0, mat_identity, sizeof(mat_identity));

        SDL_BindGPUVertexBuffers(pass, 0, &(SDL_GPUBufferBinding) {.buffer = ctx->VertexBuffer, .offset = 0}, 1);
        SDL_BindGPUIndexBuffer(pass, &(SDL_GPUBufferBinding) {.buffer = ctx->IndexBuffer, .offset = 0}, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(pass, 36, 1, 0, 0, 0);

        SDL_EndGPURenderPass(pass);

    }

    SDL_SubmitGPUCommandBuffer(cmd);
    return 0;
}

static void Quit (Context *c) {
    Cube3DContext* data = (Cube3DContext*)c->ExampleData;

    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->VertexBuffer);
    data->VertexBuffer = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->IndexBuffer);
    data->IndexBuffer = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

static void OnAppEvent(Context *c, SDL_Event *event) {
    (void)c;
    (void)event;
}

Example Cube3D_Example = {
    "Cube3D", Init, Update, Draw, Quit, OnAppEvent
};
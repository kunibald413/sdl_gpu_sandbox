#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

typedef struct BasicTriangle {
    SDL_FColor ClearColor;
    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPUBuffer* VertexBuffer;
} BasicTriangle;


// must match vertex shader inputs
// 2 floats for position
// 3 floats for color
typedef struct BasicVertex {
    float x,y;
    float r,g,b;
} BasicVertex;

static int Init(Context *c) {
    BasicTriangle* data = (BasicTriangle*)SDL_calloc(1, sizeof(BasicTriangle));
    if (!data) return 1;

    // create shaders for graphics pipeline
    SDL_GPUShader *vert = LoadShader(c->Device, "Triangle.vert", 0, 0, 0, 0);
    SDL_GPUShader *frag = LoadShader(c->Device, "ColorPassthrough.frag", 0, 0, 0, 0);

    if (!vert || !frag) {
        SDL_Log("Failed to load shaders %s", __FILE__);
        return 1;
    }

    SDL_Log("Loaded shaders %s", __FILE__);

    // gpu buffer holds data
    // we move our data with a transfer buffer to gpu memory for faster access

    // compose pipeline
    // pipeline holds the rules/description for how to draw with that data
    // need to specify how the buffer shuld be read precicely

    // vertex shader has two attributes
    const int num_vertex_attributes = 2;
    const Uint32 buffer_slot = 0;

    SDL_GPUVertexBufferDescription vert_buff_info = {0};
    vert_buff_info.slot = buffer_slot; // what is this?
    vert_buff_info.pitch = sizeof(BasicVertex);
    vert_buff_info.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX; // rate at which data is pulled(?)
    vert_buff_info.instance_step_rate = 0; // api says for future use

    SDL_Log("size of float: %zu", sizeof(float));
    SDL_Log("size of vertex: %zu", sizeof(BasicVertex));


    // descibe attributes to the pipeline
    SDL_GPUVertexAttribute vert_attributes[num_vertex_attributes];

    // position, 2 floats, (x,y)
    vert_attributes[0].location = 0; // TEXCOORD0 in hlsl
    vert_attributes[0].buffer_slot = buffer_slot;
    vert_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vert_attributes[0].offset = offsetof(BasicVertex, x); // pos data starts at 0

    // color, 3 floats, r,g,b
    vert_attributes[1].location = 1; // TEXCOORD1 in hlsl
    vert_attributes[1].buffer_slot = buffer_slot;
    vert_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vert_attributes[1].offset = offsetof(BasicVertex, r); // color data starts at 8 (2 floats)

    SDL_Log("offset pos  : %zu", offsetof(BasicVertex, x));
    SDL_Log("offset color: %zu", offsetof(BasicVertex, r));


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

    // wire up buffer
    pipe_info.vertex_input_state.vertex_buffer_descriptions = &vert_buff_info; // pointer aka array
    pipe_info.vertex_input_state.num_vertex_buffers = 1; // we have only one buffer
    pipe_info.vertex_input_state.vertex_attributes = vert_attributes;
    pipe_info.vertex_input_state.num_vertex_attributes = num_vertex_attributes;

    
    pipe_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipe_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipe_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(c->Device, &pipe_info);


    SDL_ReleaseGPUShader(c->Device, vert);
    SDL_ReleaseGPUShader(c->Device, frag);

    if (!pipe) {
        SDL_Log("Failed to create pipeline %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    // move data to gpu buffer

    // our triangle
    BasicVertex vertices[3] = {
        {-0.5, 0.5, 0.9, 0.1, 0.1},
        { 0.5, 0.5, 0.1, 0.9, 0.1},
        { 0.0, -0.5, 0.1, 0.1, 0.9}
    };

    // define buffer
    SDL_GPUBufferCreateInfo buffer_info = {0};
    buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_info.size = sizeof(vertices);

    SDL_GPUBuffer *vert_buffer = SDL_CreateGPUBuffer(c->Device, &buffer_info);
    if (!vert_buffer) {
        SDL_Log("Failed to create vertex buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    // transfer data

    // holy lord, have mercy on my soul
    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices);

    SDL_GPUTransferBuffer *transfer_buff = SDL_CreateGPUTransferBuffer(c->Device, &transfer_info);
    if (!transfer_buff) {
        SDL_Log("Failed to create transfer buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    void *mapped = SDL_MapGPUTransferBuffer(c->Device, transfer_buff, false);
    if (!mapped) {
        SDL_Log("Failed to map transfer buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }
    SDL_memcpy(mapped, vertices, sizeof(vertices));
    SDL_UnmapGPUTransferBuffer(c->Device, transfer_buff);

    SDL_GPUCommandBuffer *up_cmd = SDL_AcquireGPUCommandBuffer(c->Device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(up_cmd);
    SDL_GPUTransferBufferLocation src = {
        .transfer_buffer = transfer_buff,
        .offset = 0
    };
    SDL_GPUBufferRegion dst = {
        .buffer = vert_buffer,
        .offset = 0,
        .size = sizeof(vertices)
    };
    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(up_cmd);
    SDL_ReleaseGPUTransferBuffer(c->Device, transfer_buff);
    // end move

    data->Pipeline = pipe;
    data->VertexBuffer = vert_buffer;
    data->ClearColor = (SDL_FColor){0.0, 0.01, 0.01, 1.0};
    c->ExampleData = data;

    return 0;
}

static int Update(Context *c) {
    return 0;
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

        // bind the pipeline and the buffer
        SDL_BindGPUGraphicsPipeline(pass, data->Pipeline);
        
        SDL_GPUBufferBinding buff_bind = {0};
        buff_bind.buffer = data->VertexBuffer;
        buff_bind.offset = 0;
        SDL_BindGPUVertexBuffers(pass, 0, &buff_bind, 1);
        
        // instruct to draw
        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd);

    return 0;
}

static void Quit(Context *c) {
    BasicTriangle* data = (BasicTriangle*)c->ExampleData;
    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;
    
    SDL_ReleaseGPUBuffer(c->Device, data->VertexBuffer);
    data->VertexBuffer = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicTriangleVertexBuffer_Example = {
    "BasicTriangleVertexBuffer", Init, Update, Draw, Quit
};
#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

// or "Element Buffer"
typedef struct BasicIndexBuffer{
    SDL_GPUGraphicsPipeline* Pipeline;
    SDL_GPUBuffer* VertexBuffer;
    SDL_GPUBuffer* IndexBuffer;
} BasicIndexBuffer;


typedef struct BasicVertex {
    float x, y; // Position
} BasicVertex;

// macros wow
#define NEW(TYPE) ((TYPE*)SDL_calloc(1, sizeof(TYPE)))

static int Init(Context *c) {
    BasicIndexBuffer* data = NEW(BasicIndexBuffer);
    if (!data) return 1;

    SDL_GPUShader *vert = LoadShader(c->Device, "Passthrough2D.vert", 0, 0, 0, 0);
    SDL_GPUShader *frag = LoadShader(c->Device, "SolidColor.frag", 0, 0, 0, 0);

    if (!vert || !frag) {
        SDL_Log("Failed to load shaders %s", __FILE__);
        return 1;
    }

    const int num_vert_attributes = 1;
    const Uint32 buffer_slot = 0;

    SDL_GPUVertexBufferDescription vert_buff_info = {0};
    vert_buff_info.slot = buffer_slot;
    vert_buff_info.pitch = sizeof(BasicVertex);
    vert_buff_info.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vert_buff_info.instance_step_rate = 0;

    SDL_GPUVertexAttribute attr[num_vert_attributes];
    attr[0].location = 0;
    attr[0].buffer_slot = buffer_slot;
    attr[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    attr[0].offset = offsetof(BasicVertex, x);

    SDL_GPUGraphicsPipelineCreateInfo pipe_info = {0};
    pipe_info.vertex_input_state.vertex_buffer_descriptions = &vert_buff_info;
    pipe_info.vertex_input_state.num_vertex_buffers = 1;
    pipe_info.vertex_input_state.vertex_attributes = attr;
    pipe_info.vertex_input_state.num_vertex_attributes = num_vert_attributes;

    pipe_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipe_info.vertex_shader = vert;
    pipe_info.fragment_shader = frag;

    pipe_info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipe_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipe_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    pipe_info.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
        .num_color_targets = 1,
        .color_target_descriptions = (SDL_GPUColorTargetDescription[]) {{
            .format = SDL_GetGPUSwapchainTextureFormat(c->Device, c->Window)
        }},
    };

    SDL_GPUGraphicsPipeline *pipe = SDL_CreateGPUGraphicsPipeline(c->Device, &pipe_info);


    SDL_ReleaseGPUShader(c->Device, vert);
    SDL_ReleaseGPUShader(c->Device, frag);

    if (!pipe) {
        SDL_Log("Failed to create pipeline %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    BasicVertex vertices[4] = {
        { -0.3,  0.3},
        {  0.3,  0.3},
        { -0.3, -0.3},
        {  0.3, -0.3},
    };

    int indicies[6] = {
        0,1,2,
        1,2,3
    };

    SDL_Log("size of vertices: %zu", sizeof(vertices));
    SDL_Log("size of indicies: %zu", sizeof(indicies));

    // create the two buffers
    SDL_GPUBufferCreateInfo vert_buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = sizeof(vertices),
        .props = 0
    };
    SDL_GPUBuffer *vert_buffer = SDL_CreateGPUBuffer(c->Device, &vert_buffer_info);
    if (!vert_buffer) {
        SDL_Log("Failed to create vertex buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    SDL_GPUBufferCreateInfo idx_buffer_info = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = sizeof(indicies),
        .props = 0
    };
    SDL_GPUBuffer *idx_buffer = SDL_CreateGPUBuffer(c->Device, &idx_buffer_info);
    if (!idx_buffer) {
        SDL_Log("Failed to create index buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }

    // TRANSFER holy lord, have mercy on my soul
    // use one transfer buffer to move all the data
    SDL_GPUTransferBufferCreateInfo transfer_info = {0};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size = sizeof(vertices) + sizeof(indicies);

    SDL_GPUTransferBuffer *transfer_buff = SDL_CreateGPUTransferBuffer(c->Device, &transfer_info);
    if (!transfer_buff) {
        SDL_Log("Failed to create transfer buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }


    char *mapped = SDL_MapGPUTransferBuffer(c->Device, transfer_buff, false);
    if (!mapped) {
        SDL_Log("Failed to map transfer buffer %s %s", SDL_GetError(), __FILE__);
        return 1;
    }
    SDL_memcpy(mapped, vertices, sizeof(vertices));
    SDL_memcpy(mapped + sizeof(vertices), indicies, sizeof(indicies));

    SDL_UnmapGPUTransferBuffer(c->Device, transfer_buff);

    SDL_GPUCommandBuffer *up_cmd = SDL_AcquireGPUCommandBuffer(c->Device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(up_cmd);

    // from transferbuffer into vertex buffer
    SDL_GPUTransferBufferLocation src = { .transfer_buffer = transfer_buff, .offset = 0};
    SDL_GPUBufferRegion dst = { .buffer = vert_buffer, .offset = 0, .size = sizeof(vertices) };
    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);

    // from same transfer buffer into index buffer
    SDL_GPUTransferBufferLocation src_idx = { .transfer_buffer = transfer_buff, .offset = sizeof(vertices)};
    SDL_GPUBufferRegion dst_idx = { .buffer = idx_buffer, .offset = 0, .size = sizeof(indicies) };
    SDL_UploadToGPUBuffer(copy_pass, &src_idx, &dst_idx, false);


    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(up_cmd);
    SDL_ReleaseGPUTransferBuffer(c->Device, transfer_buff);
    // end TRANSFER


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
    BasicIndexBuffer* data = (BasicIndexBuffer*)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);
    SDL_GPUTexture *swapchain = NULL;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, c->Window, &swapchain, NULL, NULL)) {
        return 1;
    }

    int draw_indexed = 1;
    Uint32 num_instances = 1;

    if (swapchain) {
        SDL_GPUColorTargetInfo cinfo = {
            .texture = swapchain,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
            .clear_color = (SDL_FColor){0.0, 0.0, 0.0, 1.0}
        };
        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmd, &cinfo, 1, NULL);

        // bind the pipeline and the buffer
        SDL_BindGPUGraphicsPipeline(pass, data->Pipeline);
        
        SDL_GPUBufferBinding buff_bind = {0};
        buff_bind.buffer = data->VertexBuffer;
        buff_bind.offset = 0;
        SDL_BindGPUVertexBuffers(pass, 0, &buff_bind, 1);
        
        
        if (draw_indexed) {
            SDL_GPUBufferBinding buff_bind_idx = {0};
            buff_bind_idx.buffer = data->IndexBuffer;
            buff_bind_idx.offset = 0;

            // (3, 0) for one triangle
            // (3, 3) for the other
            // (6, 0) for a quad
            Uint32 num_indicies = 3; 
            Uint32 index_offset = 0;
            SDL_BindGPUIndexBuffer(pass, &buff_bind_idx, SDL_GPU_INDEXELEMENTSIZE_32BIT);
            SDL_DrawGPUIndexedPrimitives(pass, num_indicies, num_instances, index_offset, 0, 0);
        } else {
            // 0 for one triangle
            // 1 for the other
            Uint32 index_offset = 0; 
            SDL_DrawGPUPrimitives(pass, 3, 1, index_offset, 0);
        }

        SDL_EndGPURenderPass(pass);
    }

     SDL_SubmitGPUCommandBuffer(cmd);


    return 0;
}

static void Quit(Context *c) {
    BasicIndexBuffer* data = (BasicIndexBuffer*)c->ExampleData;

    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->VertexBuffer);
    data->VertexBuffer = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->IndexBuffer);
    data->IndexBuffer = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example BasicIndexBuffer_Example = {
    "BasicIndexBuffer", Init, Update, Draw, Quit
};






#include <SDL3/SDL.h>
#include "example_base.h"
#include "shaders.h"

// vert buffer
// index buffer
// mat4 uniform
// vertex: position, color (3xfloat, 3xfloat)
// pipeline
// wireframe

typedef struct PongContext {
    SDL_GPUGraphicsPipeline *Pipeline;
    SDL_GPUBuffer *VertexBuffer;
    SDL_GPUBuffer *IndexBuffer;
    Uint64 ExampleStartTicks;
    char LasteEventDesc[256];

    float PlayerY;

    float BallVelocityY;
    float BallVelocityX;
    float BallPosX;
    float BallPosY;

    float OppY;
    int PlayerScore;
    int OpponentScore;

} PongContext;


typedef struct PongVert {
    float x,y,z; // pos
    float r,g,b; // color
} PongVert;


static int Init(Context *c) {

    PongContext *ctx = (PongContext *)SDL_calloc(1, sizeof(PongContext));

    SDL_GPUVertexElementFormat attr_format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;


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
    pi.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pi.rasterizer_state.enable_depth_clip = true; // enable z/depth clipping

    const Uint16 BUFF_SLOT = 0;
    pi.vertex_input_state = (SDL_GPUVertexInputState) {
        .num_vertex_buffers = 1,
        .vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]) {{
            .slot = BUFF_SLOT,
            .pitch = sizeof(PongVert),
            .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
            .instance_step_rate = 0
        }},
        .num_vertex_attributes = 2,
        .vertex_attributes = (SDL_GPUVertexAttribute[]) {
            // pos
            {.location = 0, .offset = offsetof(PongVert, x), .buffer_slot = BUFF_SLOT, .format = attr_format},
            // color
            {.location = 1, .offset = offsetof(PongVert, r), .buffer_slot = BUFF_SLOT, .format = attr_format},
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

    PongVert verticies[4] = {
        { -0.5,  0.5,   0.0,  0.5, 0.5, 0.0}, // TOP LEFT
        {  0.5,  0.5,   0.0,  0.0, 1.0, 1.0}, // TOP RIGHT
        { -0.5, -0.5,   0.0,  1.0, 0.0, 1.0}, // BOT LEFT
        {  0.5, -0.5,   0.0,  1.0, 1.0, 0.0}, // BOT RIGHT
    };

    // 4 faces
    int indicies[2*3] = {
        0,1,2, // front
        1,2,3, // right
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
    ctx->ExampleStartTicks = SDL_GetTicks();

    ctx->PlayerY = 0.0;
    ctx->BallVelocityX = 0.005;
    ctx->BallVelocityY = 0.0001;

    c->ExampleData = ctx;

    return 0;
}

static void OnAppEvent(Context *c, SDL_Event *event) {

    PongContext* ctx = (PongContext *)c->ExampleData;

    SDL_GetEventDescription(event, ctx->LasteEventDesc, sizeof(ctx->LasteEventDesc));
    SDL_Log("Handel event: %d %s", event->type, ctx->LasteEventDesc);

    const float min = -0.75;
    const float max = 0.75;
    const float speed = 0.05;
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_DOWN) {
            ctx->PlayerY -= speed;
            if (ctx->PlayerY < min) ctx->PlayerY = min;
        }
        if (event->key.key == SDLK_UP) {
            ctx->PlayerY += speed;
            if (ctx->PlayerY > max) ctx->PlayerY = max;
        }
    }
}

static int Update(Context *c) {
    PongContext* ctx = (PongContext *)c->ExampleData;

    // opp y up and down, no tracking
    float elapsed = (float)(SDL_GetTicks() - ctx->ExampleStartTicks) / 1000.0;
    ctx->OppY = SDL_sinf(elapsed) + 0.25f;

   // based on clip-space constants as in draw
    const float ball_r = 0.05f; // scale_ball(0.1) * 0.5
    const float player_half_width = 0.25f; // scale_player(0.5) * 0.5
    const float player_center_offset = 0.70f; // as in draw

    const float left_face = -player_center_offset + player_half_width;
    const float right_face =  player_center_offset - player_half_width;
    
    ctx->BallPosX += ctx->BallVelocityX;
    ctx->BallPosY += ctx->BallVelocityY;

    // bounce off of edges
    //if (ctx->BallPosX < -1.0 || ctx->BallPosX > 1.0) ctx->BallVelocityX *= -1;
    if (ctx->BallPosY < -1.0 || ctx->BallPosY > 1.0) ctx->BallVelocityY *= -1;


    // collide left player
    if (ctx->BallVelocityX < 0 &&
        ctx->BallPosX - ball_r <= left_face &&
        ctx->BallPosX + ball_r >= -player_center_offset - player_half_width &&
        ctx->BallPosY + ball_r >= ctx->PlayerY - player_half_width &&
        ctx->BallPosY - ball_r <= ctx->PlayerY + player_half_width)
    {
        ctx->BallVelocityX *= -1;
        ctx->BallPosX = left_face + ball_r; // jump out
    }

    // collide right player
    if (ctx->BallVelocityX > 0 &&
        ctx->BallPosX + ball_r >= right_face &&
        ctx->BallPosX - ball_r <= player_center_offset + player_half_width &&
        ctx->BallPosY + ball_r >= ctx->OppY - player_half_width &&
        ctx->BallPosY - ball_r <= ctx->OppY + player_half_width)
    {
        ctx->BallVelocityX *= -1;
        ctx->BallPosX = right_face - ball_r;
    }

    // when ball entirey out of screen, score
    int scored = 0;
    if (ctx->BallPosX < -1.0f - ball_r) { // left
        ctx->OpponentScore++;
        ctx->BallVelocityX = -0.01f; // respawn towards left
        scored = 1;
    } else if (ctx->BallPosX > 1.0f + ball_r) { // right
        ctx->PlayerScore++;
        ctx->BallVelocityX = 0.01f; // towards right
        scored = 1;
    }

    if (scored) {
        ctx->BallPosX = 0;
        ctx->BallPosY = 0;
        ctx->BallVelocityY = ((SDL_rand(2) == 0) ? 1.0f : -1.0f) * 0.069691337f;
    }

    return 0;
}

static int Draw(Context *c) {

    PongContext* ctx = (PongContext *)c->ExampleData;

    SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(c->Device);

    float elapsed = (float)(SDL_GetTicks() - ctx->ExampleStartTicks) / 1000.0;
    float z_add = SDL_sinf(elapsed) * 0.5;
    //float opp_y = SDL_sinf(elapsed) + 0.25;
    float opp_y = ctx->OppY;
    z_add += 0.0;

    const float player_center_offset = 0.7;
    const float scale_player = 0.5;
    const float scale_ball = 0.1;
    
    float mat_identity[4*4] = {
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, ctx->PlayerY,
        0.0, 0.0, 0.5, 0.5,
        0.0, 0.0, 0.0, 1.0,
    };


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

        
        mat_identity[4 * 0 + 3] = -player_center_offset;
        mat_identity[4 * 0 + 0] = scale_player;
        mat_identity[4 * 1 + 1] = scale_player;
        mat_identity[4 * 2 + 2] = scale_player;
        SDL_PushGPUVertexUniformData(cmd, 0, &mat_identity, sizeof(mat_identity));
        SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 0, 0, 0);

        mat_identity[4 * 0 + 3] = player_center_offset;
        mat_identity[4 * 1 + 3] = opp_y;
        SDL_PushGPUVertexUniformData(cmd, 0, &mat_identity, sizeof(mat_identity));
        SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 0, 0, 0);
        
        mat_identity[4 * 0 + 3] = 0.0;
        mat_identity[4 * 0 + 0] = scale_ball;
        mat_identity[4 * 1 + 1] = scale_ball;
        mat_identity[4 * 2 + 2] = scale_ball;
        mat_identity[4 * 0 + 3] = ctx->BallPosX;
        mat_identity[4 * 1 + 3] = ctx->BallPosY;
        SDL_PushGPUVertexUniformData(cmd, 0, &mat_identity, sizeof(mat_identity));
        SDL_DrawGPUIndexedPrimitives(pass, 6, 1, 0, 0, 0);


        SDL_EndGPURenderPass(pass);
    }
    SDL_SubmitGPUCommandBuffer(cmd);

    char title[64];
    SDL_snprintf(title, sizeof(title), "Pong Score: %d - %d", ctx->PlayerScore, ctx->OpponentScore);
    SDL_SetWindowTitle(c->Window, title);

    return 0;
}

static void Quit(Context *c) {
    PongContext* data = (PongContext*)c->ExampleData;

    SDL_ReleaseGPUGraphicsPipeline(c->Device, data->Pipeline);
    data->Pipeline = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->VertexBuffer);
    data->VertexBuffer = NULL;

    SDL_ReleaseGPUBuffer(c->Device, data->IndexBuffer);
    data->IndexBuffer = NULL;

    SDL_free(data);
    c->ExampleData = NULL;
}

Example Pong_Example  = {
    "Pong", Init, Update, Draw, Quit, OnAppEvent
};
#define THREADS 8


// Config
cbuffer UniformBuffer : register(b0, space2)
{
    // whatever data here
    float2 Mouse;
};


[[vk::image_format("rgba8")]]
RWTexture2D<float4> image : register(u0, space1);

[numthreads(THREADS, THREADS, 1)]
void main(uint3 GlobalInvocationID : SV_DispatchThreadID)
{
    uint width;
    uint height;
    image.GetDimensions(width, height);
    if (GlobalInvocationID.x >= width || GlobalInvocationID.y >= height)
    {
        return;
    }
    //float3 color = float3(0.92f, 0.02f, 0.02f);
    float3 color = float3(0.05f, Mouse.x, Mouse.y);
    int2 coord = int2(GlobalInvocationID.xy);
    image[coord] = float4(color, 1.0f);
}

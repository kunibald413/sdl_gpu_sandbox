cbuffer Uniform : register(b0, space1) // space1 is for uniform buffers
{
    float Time;
};


static const float2 TRIANGLE_ARR[3] = {
    float2(-0.5,  0.5), // top left
    float2( 0.5,  0.5), // top right
    float2(-0.5, -0.5), // bottom left
};

struct VSOut 
{
    float4 Position : SV_Position;
};

VSOut main(uint vertexID : SV_VertexID)
{
    VSOut output;

    float2 position = TRIANGLE_ARR[vertexID % 3];

    float c = cos(Time);
    float s = sin(Time);

    output.Position = float4(
        position.x * c - position.y * s,
        position.x * s + position.y * c,
        0.0, 1.0
    );

    return output;
}
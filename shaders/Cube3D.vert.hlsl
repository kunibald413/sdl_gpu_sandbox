cbuffer Uniform : register(b0, space1)
{
    row_major float4x4 MVP;
};

struct VertIn
{
    float3 Position : TEXCOORD0;
    float3 Color    : TEXCOORD1;
};

struct VertOut
{
    float4 Position : SV_Position;
    float4 Color    : TEXCOORD0;
};

VertOut main(VertIn input)
{
    VertOut output;
    output.Position = mul(MVP, float4(input.Position, 1.0));
    output.Color = float4(input.Color, 1.0);
    return output;
}
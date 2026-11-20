struct VSInput
{
    float2 Position : TEXCOORD0;
    float3 Color    : TEXCOORD1;
};

struct VSOut 
{
    float4 Position : SV_Position;
    float4 Color    : TEXCOORD0;
};

VSOut main(VSInput input) 
{
    VSOut output;
    output.Position = float4(input.Position, 0.0, 1.0);
    output.Color = float4(input.Color, 1.0);
    return output;
}
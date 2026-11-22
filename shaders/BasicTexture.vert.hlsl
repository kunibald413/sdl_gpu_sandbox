

struct VSInput
{
    float2 Position : TEXCOORD0;
    float2 UV       : TEXCOORD1;
};

struct VSOut
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

VSOut main(VSInput input)
{
    VSOut output;
    output.Position = float4(input.Position, 0.0, 1.0);
    output.UV = input.UV;
    return output;
}
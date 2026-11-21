struct VSInput
{
    float2 Position : TEXCOORD0;
};

struct VSOut
{
    float4 Position : SV_Position;
};

VSOut main(VSInput input)
{
    VSOut output;
    output.Position = float4(input.Position, 0.0, 1.0);
    return output;
}
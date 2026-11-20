struct PSInput
{
    float4 Color : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target
{
    return input.Color;
}
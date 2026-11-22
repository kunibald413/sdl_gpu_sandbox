Texture2D     Tex2D  : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct FSInput
{
    //float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

float4 main(FSInput input) : SV_Target
{
    return Tex2D.Sample(Sampler, input.UV);
}
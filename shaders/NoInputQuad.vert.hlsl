struct VSOut 
{
    float4 Position : SV_Position;
    float4 Color    : TEXCOORD0;
};

static const float4 QUAD_ARR[6] = {
    float4(-0.5,  0.5, 0.0, 1.0), // top left
    float4( 0.5,  0.5, 0.0, 1.0), // top right
    float4(-0.5, -0.5, 0.0, 1.0), // bottom left

    float4(-0.5, -0.5, 0.0, 1.0), // bottom left
    float4( 0.5, -0.5, 0.0, 1.0), // bottom right
    float4( 0.5,  0.5, 0.0, 1.0), // top right
};

VSOut main(uint vertexID : SV_VertexID)
{
    VSOut output;
    output.Position = QUAD_ARR[vertexID % 6];
    output.Color = float4(0.1, 0.8, 0.1, 1.0);
    return output;
}
struct VSOut 
{
    float4 position : SV_Position;
    float4 color    : TEXCOORD0;
};

VSOut main(uint vertexID : SV_VertexID) 
{
    VSOut output;

    if (vertexID == 0) {
        output.position = float4(-0.5, 0.5, 0.0, 1.0);
    } else if (vertexID == 1) {
        output.position = float4(0.5, 0.5, 0.0, 1.0);
    } else {
        output.position = float4(0.0, -0.5, 0.0, 1.0);
    }

    output.color = float4(0.1, 0.2, 0.4, 1.0);
    return output;
}
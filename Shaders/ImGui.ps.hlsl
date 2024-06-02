#include "Passes/ImGuiPass.hlsl"

float4 main(VertexOutput input) : SV_Target
{
    float4 outColor = input.Color * texture0.Sample(sampler0, input.TexCoord);
    return outColor;
}
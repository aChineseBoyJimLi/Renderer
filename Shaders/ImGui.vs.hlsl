#include "Passes/ImGuiPass.hlsl"

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.Position.xy = input.Position.xy * g_Const.invDisplaySize * float2(2.0, -2.0) + float2(-1.0, 1.0);

#if SPIRV
    output.Position.xy = output.Position.xy * float2(1.0, -1.0);
#endif
    
    output.Position.zw = float2(0, 1);
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    return output;
}
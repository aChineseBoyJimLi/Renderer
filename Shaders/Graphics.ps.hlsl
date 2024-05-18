#include "Passes\GraphicsPipelinePass.hlsl"

float4 main(VertexOutput input) : SV_Target
{
    float4 bgColor = float4(0.1f, 0.1f, 0.1f, 0.1f);
    MaterialData matData = _MaterialData[input.MatIndex];
    float4 col = _MainTex[matData.TexIndex].Sample(_MainTex_Sampler, input.TexCoord) * matData.Color;
    float3 incident = normalize(-_LightData.LightDirection);
    float3 lightingColor =  _LightData.LightIntensity * _LightData.LightColor * saturate(pow(dot(input.WorldNormal, incident), matData.Smooth));
    return float4(lightingColor, 1) * col + bgColor;
}
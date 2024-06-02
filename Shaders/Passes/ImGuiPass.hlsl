#ifndef IMGUI_PASS_HLSL
#define IMGUI_PASS_HLSL

#include "../Include/CameraData.hlsli"
#include "../Include/TransformData.hlsli"

struct VertexInput
{
    float2 Position : POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color    : COLOR;
};

struct VertexOutput
{
    float4 Position         : SV_POSITION;
    float4 Color            : COLOR;
    float2 TexCoord		    : TEXCOORD0;
};

struct Constants
{
    float2 invDisplaySize;
};

ConstantBuffer<Constants> g_Const : register(b0);
sampler sampler0 : register(s0);
Texture2D texture0 : register(t0);

#endif
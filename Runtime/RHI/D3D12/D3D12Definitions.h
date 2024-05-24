#pragma once

#include "../RHIDefinitions.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <iostream>

#define OUTPUT_D3D12_FAILED_RESULT(Re)  if(FAILED(Re))\
    {\
        std::string str = std::system_category().message(Re);\
        std::wstring message(str.begin(), str.end());\
        Log::Error("[D3D12] Error: %s, In File: %s line %d", message.c_str(), __FILE__, __LINE__);\
    }

#define FENCE_INITIAL_VALUE 0
#define FENCE_COMPLETED_VALUE 1

struct RHIDepthStencilDesc;
struct RHIRasterizerDesc;
struct RHIBlendStateDesc;

namespace RHI::D3D12
{
    DXGI_FORMAT                 ConvertFormat(ERHIFormat inFormat);
    D3D12_COMMAND_LIST_TYPE     ConvertCommandQueueType(ERHICommandQueueType inType);
    D3D12_HEAP_TYPE             ConvertHeapType(ERHIResourceHeapType inType);
    D3D12_RESOURCE_STATES       ConvertResourceStates(ERHIResourceStates inState);
    D3D_PRIMITIVE_TOPOLOGY      ConvertPrimitiveType(ERHIPrimitiveType pt, uint32_t controlPoints = 0);
    D3D12_TEXTURE_ADDRESS_MODE  ConvertSamplerAddressMode(ERHISamplerAddressMode mode);
    UINT                        ConvertSamplerReductionType(ERHISamplerReductionType reductionType);
    void                        TranslateDepthStencilState(const RHIDepthStencilDesc& inState, D3D12_DEPTH_STENCIL_DESC& outState);
    void                        TranslateRasterizerState(const RHIRasterizerDesc& inState, D3D12_RASTERIZER_DESC& outState);
    void                        TranslateBlendState(const RHIBlendStateDesc& inState, D3D12_BLEND_DESC& outState);
    ERHIResourceViewType        ConvertRHIResourceViewType(D3D12_DESCRIPTOR_RANGE_TYPE inRangeType);
    ERHIResourceViewType        ConvertRHIResourceViewType(D3D12_ROOT_PARAMETER_TYPE inParameterType);
}

#pragma once

#include "../RHIDefinitions.h"
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <iostream>

#define OUTPUT_D3D12_FAILED_RESULT(Re)  if(FAILED(Re))\
    {\
        std::string message = std::system_category().message(Re);\
        Log::Error("[D3D12] Error: %s, In File: %s line %d", message.c_str(), __FILE__, __LINE__);\
    }

#define FENCE_INITIAL_VALUE 0
#define FENCE_COMPLETED_VALUE 1

namespace RHI::D3D12
{
    D3D12_COMMAND_LIST_TYPE ConvertCommandQueueType(ERHICommandQueueType inType);
}

#include "D3D12Definitions.h"
#include "D3D12PipelineState.h"
#include "../../Core/Log.h"

namespace RHI::D3D12
{
    D3D12_COMMAND_LIST_TYPE ConvertCommandQueueType(ERHICommandQueueType inType)
    {
        switch (inType)
        {
        case ERHICommandQueueType::Direct: return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case ERHICommandQueueType::Copy  : return D3D12_COMMAND_LIST_TYPE_COPY;
        case ERHICommandQueueType::Async : return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        default:
            Log::Warning("Invalid command queue type. Defaulting to D3D12_COMMAND_LIST_TYPE_DIRECT.");
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

    // D3D12_ROOT_PARAMETER ConvertDescriptorTable(const RHIPipelineBindingItem& inItem)
    // {
    //     D3D12_ROOT_PARAMETER rootParameter;
    //     rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    //     rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    //     rootParameter.DescriptorTable.
    // }
}

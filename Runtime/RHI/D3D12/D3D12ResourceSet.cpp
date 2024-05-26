#include "D3D12Resources.h"
#include "D3D12Device.h"
#include "D3D12PipelineState.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIResourceSet> D3D12Device::CreateResourceSet(const RHIPipelineBindingLayout* inLayout)
{
    RefCountPtr<RHIResourceSet> resourceSet(new D3D12ResourceSet(*this, inLayout));
    if(!resourceSet->Init())
    {
        Log::Error("[D3D12] Failed to create resource set");
    }
    return resourceSet;
}

D3D12ResourceSet::D3D12ResourceSet(D3D12Device& inDevice, const RHIPipelineBindingLayout* inLayout)
    : m_Device(inDevice)
    , m_Layout(inLayout)
    , m_LayoutD3D(nullptr)
{
    
}

D3D12ResourceSet::~D3D12ResourceSet()
{
    ShutdownInternal();
}

bool D3D12ResourceSet::Init()
{
    m_LayoutD3D = CheckCast<const D3D12PipelineBindingLayout*>(m_Layout);
    if(m_Layout == nullptr || !m_Layout->IsValid())
    {
        Log::Error("[D3D12] D3D12PipelineBindingLayout is invalid");
        return false;
    }

    m_AllocatedDescriptorRanges.clear();
    m_RootArguments.clear();

    const std::vector<CD3DX12_ROOT_PARAMETER>& rootParameters = m_LayoutD3D->GetRootParameters();
    if(!rootParameters.empty())
    {
        for(uint32_t i = 0; i < rootParameters.size(); ++i)
        {
            const CD3DX12_ROOT_PARAMETER& parameter = rootParameters[i];
            if(parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                const D3D12_DESCRIPTOR_RANGE* descriptorRange = parameter.DescriptorTable.pDescriptorRanges;
                ShaderVisibleDescriptorRange range;
                if(descriptorRange->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
                {
                    range.Heap = m_Device.GetDescriptorManager().AllocateShaderVisibleSamplers(descriptorRange->NumDescriptors, range.Slot);
                }
                else
                {
                    range.Heap = m_Device.GetDescriptorManager().AllocateShaderVisibleDescriptors(descriptorRange->NumDescriptors, range.Slot);
                }
                range.NumDescriptors = descriptorRange->NumDescriptors;
                m_AllocatedDescriptorRanges.push_back(range);

                D3D12ResourceArgument argument;
                argument.ViewType = RHI::D3D12::ConvertRHIResourceViewType(descriptorRange->RangeType);
                argument.NumDescriptors = descriptorRange->NumDescriptors;
                argument.BaseRegister = descriptorRange->BaseShaderRegister;
                argument.Space = descriptorRange->RegisterSpace;
                argument.ParameterType = parameter.ParameterType;
                argument.DescriptorTableStartHandle = range.Heap->GetGpuSlotHandle(range.Slot);
                argument.ShaderVisibleDescriptorRangeIndex = m_AllocatedDescriptorRanges.size() - 1;
                m_RootArguments.push_back(argument);
            }
            else
            {
                D3D12ResourceArgument argument;
                argument.ParameterType = parameter.ParameterType;
                argument.ViewType = RHI::D3D12::ConvertRHIResourceViewType(parameter.ParameterType);
                argument.BaseRegister = parameter.Descriptor.ShaderRegister;
                argument.Space = parameter.Descriptor.RegisterSpace;
                argument.NumDescriptors = 1;
                argument.GpuAddress = UINT64_MAX;
                m_RootArguments.push_back(argument);
            }
        }
    }
    
    return true;
}

void D3D12ResourceSet::Shutdown()
{
    ShutdownInternal();
}

bool D3D12ResourceSet::IsValid() const
{
    return m_Layout && m_Layout->IsValid();
}

void D3D12ResourceSet::ShutdownInternal()
{
    for(const auto& descriptorRange : m_AllocatedDescriptorRanges)
    {
        m_Device.GetDescriptorManager().Free(descriptorRange.Heap, descriptorRange.Slot, descriptorRange.NumDescriptors);
    }
    m_AllocatedDescriptorRanges.clear();
    m_RootArguments.clear();
}

void D3D12ResourceSet::BindBufferSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inBuffer.GetReference());
    
    if(IsValid() && buffer && buffer->IsValid())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        if(!buffer->TryGetSRVHandle(cpuDescriptorHandle))
        {
            if(!buffer->CreateSRV(cpuDescriptorHandle))
            {
                return;
            }
        }
        RHIResourceGpuAddress address = inBuffer->GetGpuAddress();
        BindResource(ERHIResourceViewType::SRV, inRegister, inSpace, cpuDescriptorHandle, address);
    }
}

void D3D12ResourceSet::BindBufferUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inBuffer.GetReference());
    
    if(IsValid() && buffer && buffer->IsValid())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        if(!buffer->TryGetUAVHandle(cpuDescriptorHandle))
        {
            if(!buffer->CreateUAV(cpuDescriptorHandle))
            {
                return;
            }
        }
        RHIResourceGpuAddress address = inBuffer->GetGpuAddress();
        BindResource(ERHIResourceViewType::UAV, inRegister, inSpace, cpuDescriptorHandle, address);
    }
}

void D3D12ResourceSet::BindBufferCBV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inBuffer.GetReference());
    
    if(IsValid() && buffer && buffer->IsValid())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        if(!buffer->TryGetCBVHandle(cpuDescriptorHandle))
        {
            if(!buffer->CreateCBV(cpuDescriptorHandle))
            {
                return;
            }
        }
        RHIResourceGpuAddress address = inBuffer->GetGpuAddress();
        BindResource(ERHIResourceViewType::CBV, inRegister, inSpace, cpuDescriptorHandle, address);
    }
}

void D3D12ResourceSet::BindTextureSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    D3D12Texture* texture = CheckCast<D3D12Texture*>(inTexture.GetReference());
    if(IsValid() && texture && texture->IsValid())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        if(!texture->TryGetSRVHandle(cpuDescriptorHandle))
        {
            if(!texture->CreateSRV(cpuDescriptorHandle))
            {
                return;
            }
        }
        BindResource(ERHIResourceViewType::SRV, inRegister, inSpace, cpuDescriptorHandle, UINT64_MAX);
    }
}

void D3D12ResourceSet::BindBufferSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    if(!IsValid() || inBuffer.empty())
        return;
    
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(inBuffer.size());
    for(uint32_t i = 0; i < inBuffer.size(); ++i)
    {
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inBuffer[i].GetReference());
        if(!buffer->TryGetSRVHandle(handles[i]))
        {
            if(!buffer->CreateSRV(handles[i]))
            {
                return;
            }
        }
    }
    BindResourceArray(ERHIResourceViewType::SRV, inBaseRegister, inSpace, handles);
}

void D3D12ResourceSet::BindBufferUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    if(!IsValid() || inBuffer.empty())
        return;
    
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(inBuffer.size());
    for(uint32_t i = 0; i < inBuffer.size(); ++i)
    {
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inBuffer[i].GetReference());
        if(!buffer->TryGetUAVHandle(handles[i]))
        {
            if(!buffer->CreateUAV(handles[i]))
            {
                return;
            }
        }
    }
    BindResourceArray(ERHIResourceViewType::UAV, inBaseRegister, inSpace, handles);
}

void D3D12ResourceSet::BindBufferCBVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    if(!IsValid() || inBuffer.empty())
        return;
    
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(inBuffer.size());
    for(uint32_t i = 0; i < inBuffer.size(); ++i)
    {
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inBuffer[i].GetReference());
        if(!buffer->TryGetCBVHandle(handles[i]))
        {
            if(!buffer->CreateCBV(handles[i]))
            {
                return;
            }
        }
    }
    BindResourceArray(ERHIResourceViewType::CBV, inBaseRegister, inSpace, handles);
}

void D3D12ResourceSet::BindTextureSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures)
{
    if(!IsValid() || inTextures.empty())
        return;
    
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(inTextures.size());
    for(uint32_t i = 0; i < inTextures.size(); ++i)
    {
        D3D12Texture* texture = CheckCast<D3D12Texture*>(inTextures[i].GetReference());
        if(!texture->TryGetSRVHandle(handles[i]))
        {
            if(!texture->CreateSRV(handles[i]))
            {
                return;
            }
        }
    }
    BindResourceArray(ERHIResourceViewType::SRV, inBaseRegister, inSpace, handles);
}

void D3D12ResourceSet::BindTextureUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures)
{
    if(!IsValid() || inTextures.empty())
        return;
    
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(inTextures.size());
    for(uint32_t i = 0; i < inTextures.size(); ++i)
    {
        D3D12Texture* texture = CheckCast<D3D12Texture*>(inTextures[i].GetReference());
        if(!texture->TryGetUAVHandle(handles[i]))
        {
            if(!texture->CreateUAV(handles[i]))
            {
                return;
            }
        }
    }
    BindResourceArray(ERHIResourceViewType::UAV, inBaseRegister, inSpace, handles);
}

void D3D12ResourceSet::BindSamplerArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHISampler>>& inSampler)
{
    if(!IsValid() || inSampler.empty())
        return;
    
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> handles(inSampler.size());
    for(uint32_t i = 0; i < inSampler.size(); ++i)
    {
        D3D12Sampler* sampler = CheckCast<D3D12Sampler*>(inSampler[i].GetReference());
        handles[i] = sampler->GetHandle();
    }
    BindResourceArray(ERHIResourceViewType::UAV, inBaseRegister, inSpace, handles);
}


void D3D12ResourceSet::BindTextureUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    D3D12Texture* texture = CheckCast<D3D12Texture*>(inTexture.GetReference());
    if(IsValid() && texture && texture->IsValid())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        if(!texture->TryGetUAVHandle(cpuDescriptorHandle))
        {
            if(!texture->CreateUAV(cpuDescriptorHandle))
            {
                return;
            }
        }
        BindResource(ERHIResourceViewType::UAV, inRegister, inSpace, cpuDescriptorHandle, UINT64_MAX);
    }
}

void D3D12ResourceSet::BindSampler(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHISampler>& inSampler)
{
    D3D12Sampler* sampler = CheckCast<D3D12Sampler*>(inSampler.GetReference());
    if(IsValid() && sampler && sampler->IsValid())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle = sampler->GetHandle();
        BindResource(ERHIResourceViewType::Sampler, inRegister, inSpace, cpuDescriptorHandle, UINT64_MAX);
    }
}

void D3D12ResourceSet::BindResource(ERHIResourceViewType inViewType, uint32_t inRegister, uint32_t inSpace, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, RHIResourceGpuAddress address)
{
    for(uint32_t i = 0; i < m_RootArguments.size(); ++i)
    {
        D3D12ResourceArgument& argument = m_RootArguments[i];
        if(argument.ViewType == inViewType && argument.NumDescriptors == 1 && argument.BaseRegister == inRegister && argument.Space == inSpace)
        {
            if(argument.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            {
                ShaderVisibleDescriptorRange& range = m_AllocatedDescriptorRanges[argument.ShaderVisibleDescriptorRangeIndex];
                m_Device.GetDescriptorManager().CopyDescriptors(range.Heap, 1, range.Slot, cpuDescriptorHandle);
            }
            else
            {
                argument.GpuAddress = address;
            }
            break;
        }
    }
}

void D3D12ResourceSet::BindResourceArray(ERHIResourceViewType inViewType, uint32_t inBaseRegister, uint32_t inSpace, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& cpuDescriptorHandles)
{
    for(uint32_t i = 0; i < m_RootArguments.size(); ++i)
    {
        D3D12ResourceArgument& argument = m_RootArguments[i];
        if(argument.ViewType == inViewType && argument.BaseRegister == inBaseRegister && argument.NumDescriptors >= cpuDescriptorHandles.size()  && argument.Space == inSpace)
        {
            for(uint32_t i = 0; i < cpuDescriptorHandles.size(); i++)
            {
                ShaderVisibleDescriptorRange& range = m_AllocatedDescriptorRanges[argument.ShaderVisibleDescriptorRangeIndex];
                m_Device.GetDescriptorManager().CopyDescriptors(range.Heap, 1, range.Slot + i, cpuDescriptorHandles[i]);
            }
            break;
        }
    }
}

void D3D12ResourceSet::SetGraphicsRootArguments(ID3D12GraphicsCommandList* inCmdList) const
{
    if(IsValid())
    {
        const std::vector<CD3DX12_ROOT_PARAMETER>& rootParameters = m_LayoutD3D->GetRootParameters();
        if(!rootParameters.empty())
        {
            for(uint32_t i = 0; i < rootParameters.size(); ++i)
            {
                const CD3DX12_ROOT_PARAMETER& parameter = rootParameters[i];
                switch(parameter.ParameterType)
                {
                case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                    inCmdList->SetGraphicsRootDescriptorTable(i, m_RootArguments[i].DescriptorTableStartHandle);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_CBV:
                    inCmdList->SetGraphicsRootConstantBufferView(i, m_RootArguments[i].GpuAddress);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_SRV:
                    inCmdList->SetGraphicsRootShaderResourceView(i, m_RootArguments[i].GpuAddress);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_UAV:
                    inCmdList->SetGraphicsRootUnorderedAccessView(i, m_RootArguments[i].GpuAddress);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                    Log::Error("[D3D12] Not support root constants");
                    return;
                }
            }
        }
    }
    else
    {
        Log::Error("[D3D12] ResourceSet is invalid");
    }
}

void D3D12ResourceSet::SetComputeRootArguments(ID3D12GraphicsCommandList* inCmdList) const
{
    if(IsValid())
    {
        const std::vector<CD3DX12_ROOT_PARAMETER>& rootParameters = m_LayoutD3D->GetRootParameters();
        if(!rootParameters.empty())
        {
            for(uint32_t i = 0; i < rootParameters.size(); ++i)
            {
                const CD3DX12_ROOT_PARAMETER& parameter = rootParameters[i];
                switch(parameter.ParameterType)
                {
                case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
                    inCmdList->SetComputeRootDescriptorTable(i, m_RootArguments[i].DescriptorTableStartHandle);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_CBV:
                    inCmdList->SetComputeRootConstantBufferView(i, m_RootArguments[i].GpuAddress);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_SRV:
                    inCmdList->SetComputeRootShaderResourceView(i, m_RootArguments[i].GpuAddress);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_UAV:
                    inCmdList->SetComputeRootUnorderedAccessView(i, m_RootArguments[i].GpuAddress);
                    break;
                case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
                    Log::Error("[D3D12] Not support root constants");
                    return;
                }
            }
        }
    }
    else
    {
        Log::Error("[D3D12] ResourceSet is invalid");
    }
}

void D3D12ResourceSet::BindAccelerationStructure(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIAccelerationStructure>& inAccelerationStructure)
{
    D3D12AccelerationStructure* accelerationStructure = CheckCast<D3D12AccelerationStructure*>(inAccelerationStructure.GetReference());
    if(IsValid() && accelerationStructure && accelerationStructure->IsValid() && accelerationStructure->IsTopLevel())
    {
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        RefCountPtr<D3D12Buffer> buffer = accelerationStructure->GetAccelerationStructure();
        if(!buffer->TryGetSRVHandle(cpuDescriptorHandle))
        {
            if(!buffer->CreateSRV(cpuDescriptorHandle))
            {
                return;
            }
        }
        RHIResourceGpuAddress address = buffer->GetGpuAddress();
        BindResource(ERHIResourceViewType::SRV, inRegister, inSpace, cpuDescriptorHandle, address);
    }
}
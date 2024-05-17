#include "D3D12Resources.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

D3D12Sampler::D3D12Sampler(D3D12Device& inDevice, const RHISamplerDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_SamplerView()
{
    
}

D3D12Sampler::~D3D12Sampler()
{
    ShutdownInternal();
}

bool D3D12Sampler::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Sampler already initialized.");
        return true;
    }

    m_SamplerView.DescriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 1, m_SamplerView.Slot);
    if(m_SamplerView.DescriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate descriptor for sampler");
        return false;
    }

    D3D12_SAMPLER_DESC samplerDesc{};

    const uint32_t reductionType = RHI::D3D12::ConvertSamplerReductionType(m_Desc.ReductionType);

    if (m_Desc.MaxAnisotropy > 1.0f)
    {
        samplerDesc.Filter = D3D12_ENCODE_ANISOTROPIC_FILTER(reductionType);
        samplerDesc.MaxAnisotropy = static_cast<uint32_t>(m_Desc.MaxAnisotropy);
    }
    else
    {
        samplerDesc.Filter = D3D12_ENCODE_BASIC_FILTER(
            m_Desc.MinFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
            m_Desc.MagFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
            m_Desc.MipFilter ? D3D12_FILTER_TYPE_LINEAR : D3D12_FILTER_TYPE_POINT,
            reductionType);
        samplerDesc.MaxAnisotropy = 1;
    }

    samplerDesc.AddressU = RHI::D3D12::ConvertSamplerAddressMode(m_Desc.AddressU);
    samplerDesc.AddressV = RHI::D3D12::ConvertSamplerAddressMode(m_Desc.AddressV);
    samplerDesc.AddressW = RHI::D3D12::ConvertSamplerAddressMode(m_Desc.AddressW);
    samplerDesc.MipLODBias = m_Desc.MipBias;
   
    samplerDesc.BorderColor[0] = m_Desc.BorderColor[0];
    samplerDesc.BorderColor[1] = m_Desc.BorderColor[1];
    samplerDesc.BorderColor[2] = m_Desc.BorderColor[2];
    samplerDesc.BorderColor[3] = m_Desc.BorderColor[3];
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
    
    auto handle = m_SamplerView.GetCpuHande();
    m_Device.GetDevice()->CreateSampler(&samplerDesc, handle);
    
    return true;
}

void D3D12Sampler::Shutdown()
{
    ShutdownInternal();
}

bool D3D12Sampler::IsValid() const
{
    return m_SamplerView.DescriptorHeap != nullptr;
}

void D3D12Sampler::ShutdownInternal()
{
    if(IsValid())
    {
        m_Device.GetDescriptorManager().Free(m_SamplerView.DescriptorHeap, m_SamplerView.Slot, 1);
        m_SamplerView.DescriptorHeap = nullptr;
        m_SamplerView.Slot = 0;
    }
}

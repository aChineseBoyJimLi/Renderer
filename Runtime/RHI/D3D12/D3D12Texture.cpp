#include "D3D12CommandList.h"
#include "D3D12Resources.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHITexture> D3D12Device::CreateTexture(const RHITextureDesc& inDesc, bool isVirtual)
{
    RefCountPtr<RHITexture> texture(new D3D12Texture(*this, inDesc, isVirtual));
    if(!texture->Init())
    {
        Log::Error("[D3D12] Failed to create texture");
    }
    return texture;
}

RefCountPtr<D3D12Texture> D3D12Device::CreateTexture(const RHITextureDesc& inDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& inResource)
{
    return new D3D12Texture(*this, inDesc, inResource);
}

D3D12Texture::D3D12Texture(D3D12Device& inDevice, const RHITextureDesc& inDesc, bool inIsVirtual)
    : IsVirtualTexture(inIsVirtual)
    , IsManagedTexture(true)
    , m_Device(inDevice)
    , m_Desc(inDesc)
    , m_InitialStates(D3D12_RESOURCE_STATE_COMMON)
    , m_TextureHandle(nullptr)
    , m_TextureDescD3D()
    , m_AllocationInfo {inDesc.Width * inDesc.Height * inDesc.Depth * 4,  0}
    , m_ResourceHeap(nullptr)
    , m_OffsetInHeap(0)
{
    
}

D3D12Texture::D3D12Texture(D3D12Device& inDevice, const RHITextureDesc& inDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& inTexture)
    : IsVirtualTexture(false)
    , IsManagedTexture(false)
    , m_Device(inDevice)
    , m_Desc(inDesc)
    , m_InitialStates(D3D12_RESOURCE_STATE_COMMON)
    , m_TextureHandle(inTexture)
    , m_AllocationInfo {inDesc.Width * inDesc.Height * inDesc.Depth * 4,  0}
    , m_ResourceHeap(nullptr)
    , m_OffsetInHeap(0)
{
    m_TextureDescD3D = inTexture->GetDesc();
}

D3D12Texture::~D3D12Texture()
{
    ShutdownInternal();
}

bool D3D12Texture::Init()
{
    if(!IsManaged())
    {
        Log::Warning("[D3D12] Texture is not managed, initialization is not allowed.");
        return true;
    }
    
    if(IsValid())
    {
        Log::Warning("[D3D12] Texture already initialized.");
        return true;
    }

    // m_InitialStates = RHI::D3D12::ConvertResourceStates(m_Desc.InitialState);

    m_InitialStates = D3D12_RESOURCE_STATE_COMMON;
    m_TextureDescD3D.Format = RHI::D3D12::ConvertFormat(m_Desc.Format);
    switch (m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        m_TextureDescD3D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        m_TextureDescD3D.DepthOrArraySize = 1;
        break;
    case ERHITextureDimension::Texture3D:
        m_TextureDescD3D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        m_TextureDescD3D.DepthOrArraySize = static_cast<uint16_t>(m_Desc.Depth);
        break;
    case ERHITextureDimension::Texture2DArray:
        m_TextureDescD3D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        m_TextureDescD3D.DepthOrArraySize = static_cast<uint16_t>(m_Desc.ArraySize);
        break;
    case ERHITextureDimension::TextureCube:
        m_TextureDescD3D.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        m_TextureDescD3D.DepthOrArraySize = 6;
        break;
    }
    m_TextureDescD3D.Width = m_Desc.Width;
    m_TextureDescD3D.Height = m_Desc.Height;
    m_TextureDescD3D.MipLevels = (uint16_t)m_Desc.MipLevels;
    m_TextureDescD3D.SampleDesc.Count = m_Desc.SampleCount;
    m_TextureDescD3D.SampleDesc.Quality = 0;
    m_TextureDescD3D.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    m_TextureDescD3D.Flags = D3D12_RESOURCE_FLAG_NONE;

    bool hasClearValue = false;
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = m_TextureDescD3D.Format;
    if((m_Desc.Usages & ERHITextureUsage::RenderTarget) == ERHITextureUsage::RenderTarget)
    {
        m_TextureDescD3D.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        hasClearValue = true;
        clearValue.Color[0] = m_Desc.ClearValue.Color[0];
        clearValue.Color[1] = m_Desc.ClearValue.Color[1];
        clearValue.Color[2] = m_Desc.ClearValue.Color[2];
        clearValue.Color[3] = m_Desc.ClearValue.Color[3];
    }
    else if((m_Desc.Usages & ERHITextureUsage::DepthStencil) == ERHITextureUsage::DepthStencil)
    {
        m_TextureDescD3D.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        m_InitialStates = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        hasClearValue = true;
        clearValue.DepthStencil.Depth = m_Desc.ClearValue.DepthStencil.Depth;
        clearValue.DepthStencil.Stencil = m_Desc.ClearValue.DepthStencil.Stencil;
    }
        
    if((m_Desc.Usages & ERHITextureUsage::UnorderedAccess) == ERHITextureUsage::UnorderedAccess)
    {
        m_TextureDescD3D.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    m_ResourceHeap.SafeRelease();
    m_RenderTargetViews.clear();
    m_DepthStencilViews.clear();
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();

    // D3D12_CLEAR_VALUE* clearValue{};
    
    if(!IsVirtual() && IsManaged())
    {
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
        const CD3DX12_HEAP_PROPERTIES heapProp(heapType);
        HRESULT hr = m_Device.GetDevice()->CreateCommittedResource(&heapProp
            , D3D12_HEAP_FLAG_NONE
            , &m_TextureDescD3D
            , m_InitialStates
            , hasClearValue ? &clearValue : nullptr
            , IID_PPV_ARGS(&m_TextureHandle));
        
        if(FAILED(hr))
        {
            OUTPUT_D3D12_FAILED_RESULT(hr)
            return false;
        }
    }

    m_AllocationInfo = m_Device.GetDevice()->GetResourceAllocationInfo(D3D12Device::GetNodeMask(), 1, &m_TextureDescD3D);
    
    return true;
}

void D3D12Texture::Shutdown()
{
    ShutdownInternal();
}

void D3D12Texture::ShutdownInternal()
{
    for(auto rtv : m_RenderTargetViews)
    {
        m_Device.GetDescriptorManager().Free(rtv.second.DescriptorHeap, rtv.second.Slot, 1);
    }
    for(auto dsv : m_DepthStencilViews)
    {
        m_Device.GetDescriptorManager().Free(dsv.second.DescriptorHeap, dsv.second.Slot, 1);
    }
    for(auto srv : m_ShaderResourceViews)
    {
        m_Device.GetDescriptorManager().Free(srv.second.DescriptorHeap, srv.second.Slot, 1);
    }
    for(auto uav : m_UnorderedAccessViews)
    {
        m_Device.GetDescriptorManager().Free(uav.second.DescriptorHeap, uav.second.Slot, 1);
    }
    m_RenderTargetViews.clear();
    m_DepthStencilViews.clear();
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();
    
    if(m_ResourceHeap != nullptr)
    {
        m_ResourceHeap->Free(m_OffsetInHeap, GetAllocSizeInByte());
        m_ResourceHeap.SafeRelease();
        m_OffsetInHeap = 0;
    }
    
    m_TextureHandle.Reset();
}

bool D3D12Texture::BindMemory(RefCountPtr<RHIResourceHeap> inHeap)
{
    if(!IsManaged())
    {
        Log::Warning("[D3D12] Texture is not managed, memory binding is not allowed.");
        return true;    
    }
    
    if(!IsVirtual())
    {
        Log::Warning("[D3D12] Texture is not virtual, memory binding is not allowed.");
        return true;
    }

    if(m_ResourceHeap != nullptr)
    {
        Log::Warning("[D3D12] Texture already bound to a heap.");
        return true;
    }

    D3D12ResourceHeap* heap = CheckCast<D3D12ResourceHeap*>(inHeap.GetReference());

    if(heap == nullptr || !heap->IsValid())
    {
        Log::Error("[D3D12] Failed to bind texture memory, the heap is invalid");
        return false;
    }

    const RHIResourceHeapDesc& heapDesc = heap->GetDesc();

    if(heapDesc.Usage != ERHIHeapUsage::Texture)
    {
        Log::Error("[D3D12] Failed to bind texture memory, the heap is not a texture heap");
        return false;
    }

    if(!inHeap->TryAllocate(GetAllocSizeInByte(), m_OffsetInHeap))
    {
        Log::Error("[D3D12] Failed to bind texture memory, the heap size is not enough");
        return false;
    }

    bool hasClearValue = false;
    D3D12_CLEAR_VALUE clearValue;
    clearValue.Format = m_TextureDescD3D.Format;
    if((m_Desc.Usages & ERHITextureUsage::RenderTarget) == ERHITextureUsage::RenderTarget)
    {
        hasClearValue = true;
        clearValue.Color[0] = m_Desc.ClearValue.Color[0];
        clearValue.Color[1] = m_Desc.ClearValue.Color[1];
        clearValue.Color[2] = m_Desc.ClearValue.Color[2];
        clearValue.Color[3] = m_Desc.ClearValue.Color[3];
    }
    else if((m_Desc.Usages & ERHITextureUsage::DepthStencil) == ERHITextureUsage::DepthStencil)
    {
        hasClearValue = true;
        clearValue.DepthStencil.Depth = m_Desc.ClearValue.DepthStencil.Depth;
        clearValue.DepthStencil.Stencil = m_Desc.ClearValue.DepthStencil.Stencil;
    }

    HRESULT hr = m_Device.GetDevice()->CreatePlacedResource(heap->GetHeap()
        , m_OffsetInHeap
        , &m_TextureDescD3D
        , m_InitialStates
        , hasClearValue ? &clearValue : nullptr
        , IID_PPV_ARGS(&m_TextureHandle));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        inHeap->Free(m_OffsetInHeap, GetAllocSizeInByte());
        m_OffsetInHeap = 0;
        return false;
    }

    m_ResourceHeap = inHeap;
    return true;
}

bool D3D12Texture::IsValid() const
{
    bool valid = m_TextureHandle != nullptr;
    if(IsManaged() && IsVirtual())
    {
        valid = valid && m_ResourceHeap != nullptr && m_ResourceHeap->IsValid();
    }
    return valid;
}

void D3D12Texture::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_TextureHandle->SetName(name.c_str());
    }
}

bool D3D12Texture::CreateRTV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Texture is not valid.");
        return false;
    }

    if(TryGetRTVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Render target view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::RenderTarget) == 0)
    {
        Log::Error("[D3D12] Texture is not allowed to create render target view.");
        return false;
    }

    uint32_t firstMipLevel = inSubResource.FirstMipSlice;
    uint32_t mipLevels = inSubResource.NumMipSlices;
    uint32_t firstArraySlice = inSubResource.FirstArraySlice;
    uint32_t arraySize = inSubResource.NumArraySlices;
    if(inSubResource == RHITextureSubResource::All)
    {
        firstMipLevel = 0;
        mipLevels = m_Desc.MipLevels;
        firstArraySlice = 0;
        arraySize = m_Desc.ArraySize;
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the rtv");
        return false;
    }

    outHandle = descriptorHeap->GetCpuSlotHandle(slot);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = RHI::D3D12::ConvertFormat(m_Desc.Format);
    switch(m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        rtvDesc.Texture2D.MipSlice = firstMipLevel;
        rtvDesc.Texture2D.PlaneSlice = 0;
        break;
    case ERHITextureDimension::Texture2DArray:
        rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        rtvDesc.Texture2DArray.MipSlice = firstMipLevel;
        rtvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
        rtvDesc.Texture2DArray.ArraySize = arraySize;
        rtvDesc.Texture2DArray.PlaneSlice = 0;
        break;
    case ERHITextureDimension::Texture3D:
    case ERHITextureDimension::TextureCube:
        Log::Error("[D3D12] Failed to create rtv on a 3D or cube texture");
        return false;
    }

    m_Device.GetDevice()->CreateRenderTargetView(m_TextureHandle.Get(), &rtvDesc, outHandle);

    m_RenderTargetViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::RTV
        , rtvDesc));
    
    return true;
}

bool D3D12Texture::CreateDSV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Texture is not valid.");
        return false;
    }

    if(TryGetDSVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Depth stencil view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::DepthStencil) == 0)
    {
        Log::Error("[D3D12] Texture is not allowed to create depth stencil view.");
        return false;
    }

    uint32_t firstMipLevel = inSubResource.FirstMipSlice;
    uint32_t mipLevels = inSubResource.NumMipSlices;
    uint32_t firstArraySlice = inSubResource.FirstArraySlice;
    uint32_t arraySize = inSubResource.NumArraySlices;
    if(inSubResource == RHITextureSubResource::All)
    {
        firstMipLevel = 0;
        mipLevels = m_Desc.MipLevels;
        firstArraySlice = 0;
        arraySize = m_Desc.ArraySize;
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the dsv");
        return false;
    }

    outHandle = descriptorHeap->GetCpuSlotHandle(slot);
    
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
    dsvDesc.Format = RHI::D3D12::ConvertFormat(m_Desc.Format);
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    switch (m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = firstMipLevel;
        break;
    case ERHITextureDimension::Texture2DArray:
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
        dsvDesc.Texture2DArray.MipSlice = firstMipLevel;
        dsvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
        dsvDesc.Texture2DArray.ArraySize = arraySize;
        break;
    case ERHITextureDimension::Texture3D:
    case ERHITextureDimension::TextureCube:
        Log::Error("[D3D12] Failed to create dsv on a 3D or cube texture");
        return false;
    }

    m_Device.GetDevice()->CreateDepthStencilView(m_TextureHandle.Get(), &dsvDesc, outHandle);

    m_DepthStencilViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::DSV
        , dsvDesc));
    
    return true;
}

bool D3D12Texture::CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Texture is not valid.");
        return false;
    }

    if(TryGetSRVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Shader resource view already created on this subresource.");
        return true;
    }

    uint32_t firstMipLevel = inSubResource.FirstMipSlice;
    uint32_t mipLevels = inSubResource.NumMipSlices;
    uint32_t firstArraySlice = inSubResource.FirstArraySlice;
    uint32_t arraySize = inSubResource.NumArraySlices;
    if(inSubResource == RHITextureSubResource::All)
    {
        firstMipLevel = 0;
        mipLevels = m_Desc.MipLevels;
        firstArraySlice = 0;
        arraySize = m_Desc.ArraySize;
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the srv");
        return false;
    }

    outHandle = descriptorHeap->GetCpuSlotHandle(slot);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = RHI::D3D12::ConvertFormat(m_Desc.Format);
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    switch(m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = mipLevels;
        srvDesc.Texture2D.MostDetailedMip = firstMipLevel;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0;
        break;
    case ERHITextureDimension::Texture2DArray:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = firstMipLevel;
        srvDesc.Texture2DArray.MipLevels = mipLevels;
        srvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
        srvDesc.Texture2DArray.ArraySize = arraySize;
        srvDesc.Texture2DArray.PlaneSlice = 0;
        srvDesc.Texture2DArray.ResourceMinLODClamp = 0;
        break;
    case ERHITextureDimension::Texture3D:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        srvDesc.Texture3D.MostDetailedMip = firstMipLevel;
        srvDesc.Texture3D.MipLevels = mipLevels;
        srvDesc.Texture3D.ResourceMinLODClamp = 0;
        break;
    case ERHITextureDimension::TextureCube:
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDesc.TextureCube.MipLevels = mipLevels;
        srvDesc.TextureCube.MostDetailedMip = firstMipLevel;
        srvDesc.TextureCube.ResourceMinLODClamp = 0;
        break;
    }

    m_Device.GetDevice()->CreateShaderResourceView(m_TextureHandle.Get(), &srvDesc, outHandle);

    m_ShaderResourceViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::SRV
        , srvDesc));
    
    return true;
}

bool D3D12Texture::CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Texture is not valid.");
        return false;
    }

    if(TryGetUAVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Unordered access view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::UnorderedAccess) == 0)
    {
        Log::Error("[D3D12] Texture is not allowed to create unordered access view.");
        return false;
    }

    uint32_t firstMipLevel = inSubResource.FirstMipSlice;
    uint32_t mipLevels = inSubResource.NumMipSlices;
    uint32_t firstArraySlice = inSubResource.FirstArraySlice;
    uint32_t arraySize = inSubResource.NumArraySlices;
    if(inSubResource == RHITextureSubResource::All)
    {
        firstMipLevel = 0;
        mipLevels = m_Desc.MipLevels;
        firstArraySlice = 0;
        arraySize = m_Desc.ArraySize;
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the srv");
        return false;
    }

    outHandle = descriptorHeap->GetCpuSlotHandle(slot);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = RHI::D3D12::ConvertFormat(m_Desc.Format);
    switch(m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = firstMipLevel;
        uavDesc.Texture2D.PlaneSlice = 0;
        break;
    case ERHITextureDimension::Texture2DArray:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        uavDesc.Texture2DArray.MipSlice = firstMipLevel;
        uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
        uavDesc.Texture2DArray.ArraySize = arraySize;
        uavDesc.Texture2DArray.PlaneSlice = 0;
        break;
    case ERHITextureDimension::Texture3D:
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        uavDesc.Texture3D.FirstWSlice = firstArraySlice;
        uavDesc.Texture3D.MipSlice = firstMipLevel;
        uavDesc.Texture3D.WSize = arraySize;
        break;
    case ERHITextureDimension::TextureCube:
        Log::Error("[D3D12] Failed to create dsv on a cube texture");
        return false;
    }

    m_Device.GetDevice()->CreateUnorderedAccessView(m_TextureHandle.Get(), nullptr, &uavDesc, outHandle);

    m_UnorderedAccessViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::UAV
        , uavDesc));
    
    return true;
}

bool D3D12Texture::TryGetRTVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_RenderTargetViews.find(inSubResource) == m_RenderTargetViews.end())
    {
        return false;
    }
    outHandle = m_RenderTargetViews[inSubResource].GetCpuHande();
    return true;
}

bool D3D12Texture::TryGetDSVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_DepthStencilViews.find(inSubResource) == m_DepthStencilViews.end())
    {
        return false;
    }
    outHandle = m_DepthStencilViews[inSubResource].GetCpuHande();
    return true;
}

bool D3D12Texture::TryGetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_ShaderResourceViews.find(inSubResource) == m_ShaderResourceViews.end())
    {
        return false;
    }
    outHandle = m_ShaderResourceViews[inSubResource].GetCpuHande();
    return true;
}

bool D3D12Texture::TryGetUAVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_UnorderedAccessViews.find(inSubResource) == m_UnorderedAccessViews.end())
    {
        return false;
    }
    outHandle = m_UnorderedAccessViews[inSubResource].GetCpuHande();
    return true;
}

D3D12_RESOURCE_STATES D3D12Texture::GetCurrentState(const RHITextureSubResource& inSubResource)
{
    auto currentStateIter = m_SubResourceStates.find(inSubResource);
    if(currentStateIter == m_SubResourceStates.end())
    {
        auto allSubResourceState = m_SubResourceStates.find(RHITextureSubResource::All);
        if(allSubResourceState == m_SubResourceStates.end())
        {
            m_SubResourceStates.emplace(inSubResource, m_InitialStates);
            if(inSubResource != RHITextureSubResource::All)
                m_SubResourceStates.emplace(inSubResource, m_InitialStates);
            return m_InitialStates;
        }
        m_SubResourceStates.emplace(inSubResource, allSubResourceState->second);
        return allSubResourceState->second;
    }
    return currentStateIter->second;
}

void D3D12Texture::ChangeState(D3D12_RESOURCE_STATES inAfterState, const RHITextureSubResource& inSubResource)
{
    auto currentStateIter = m_SubResourceStates.find(inSubResource);
    if(currentStateIter == m_SubResourceStates.end())
    {
        m_SubResourceStates.emplace(inSubResource, inAfterState);
    }
    else
    {
        currentStateIter->second = inAfterState;
    }
}

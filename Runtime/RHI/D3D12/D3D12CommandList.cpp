#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12PipelineState.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"
#include <pix.h>

RefCountPtr<RHICommandList> D3D12Device::CreateCommandList(ERHICommandQueueType inType)
{
    RefCountPtr<RHICommandList> cmdList(new D3D12CommandList(*this, inType));
    if(!cmdList->Init())
    {
        Log::Error("[D3D12] Failed to create a command list");
    }
    return cmdList;
}

D3D12CommandList::D3D12CommandList(D3D12Device& inDevice, ERHICommandQueueType inType)
    : m_Device(inDevice)
    , m_Context()
    , m_QueueType(inType)
    , m_CmdAllocatorHandle(nullptr)
    , m_CmdListHandle(nullptr)
    , m_IsClosed(false)
{
    
}

D3D12CommandList::~D3D12CommandList()
{
    ShutdownInternal();
}

bool D3D12CommandList::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Command list is already initialized");
        return true;
    }

    m_CachedBarriers.clear();
    m_StagingBuffers.clear();

    const D3D12_COMMAND_LIST_TYPE d3dQueueType = RHI::D3D12::ConvertCommandQueueType(m_QueueType);

    HRESULT hr = m_Device.GetDevice()->CreateCommandAllocator(d3dQueueType, IID_PPV_ARGS(&m_CmdAllocatorHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    
    hr = m_Device.GetDevice()->CreateCommandList(D3D12Device::GetNodeMask(), d3dQueueType, m_CmdAllocatorHandle.Get(), nullptr, IID_PPV_ARGS(&m_CmdListHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    return true;
}

void D3D12CommandList::Shutdown()
{
    ShutdownInternal();
}

bool D3D12CommandList::IsValid() const
{
    bool valid = m_CmdAllocatorHandle != nullptr && m_CmdListHandle != nullptr;
    return valid;
}

void D3D12CommandList::Begin()
{
    if(IsValid() && m_IsClosed)
    {
        m_CmdAllocatorHandle->Reset();
        m_CmdListHandle->Reset(m_CmdAllocatorHandle.Get(), nullptr);
        m_IsClosed = false;
    }
}

void D3D12CommandList::End()
{
    if(IsValid() && !m_IsClosed)
    {
        FlushBarriers();
        m_CmdListHandle->Close();
        m_IsClosed = true;
        m_Context.Clear();
    }
}

void D3D12CommandList::BeginMark(const char* name)
{
    PIXBeginEvent(m_CmdListHandle.Get(), 0, name);
}

void D3D12CommandList::EndMark()
{
    PIXEndEvent(m_CmdListHandle.Get());
}

void D3D12CommandList::SetPipelineState(const RefCountPtr<RHIComputePipeline>& inPipelineState)
{
    if(IsValid() && !m_IsClosed)
    {
        const D3D12ComputePipeline* pipeline = CheckCast<D3D12ComputePipeline*>(inPipelineState.GetReference());
        const D3D12PipelineBindingLayout* layout = CheckCast<D3D12PipelineBindingLayout*>(pipeline->GetDesc().BindingLayout);
        SetPipelineState(pipeline ? pipeline->GetPipelineState() : nullptr
                , layout ? layout->GetRootSignature() : nullptr);
    }
}

void D3D12CommandList::SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState)
{
    if(IsValid() && !m_IsClosed)
    {
        const D3D12GraphicsPipeline* pipeline = CheckCast<D3D12GraphicsPipeline*>(inPipelineState.GetReference());
        const D3D12PipelineBindingLayout* layout = CheckCast<D3D12PipelineBindingLayout*>(pipeline->GetDesc().BindingLayout);
        if(pipeline && pipeline->IsValid())
        {
            const RHIGraphicsPipelineDesc& pipelineDesc = pipeline->GetDesc();
            if(!pipelineDesc.UsingMeshShader)
                m_CmdListHandle->IASetPrimitiveTopology(RHI::D3D12::ConvertPrimitiveType(pipelineDesc.PrimitiveType));
        }
        SetPipelineState(pipeline ? pipeline->GetPipelineState() : nullptr
                , layout ? layout->GetRootSignature() : nullptr);
    }
}

void D3D12CommandList::SetPipelineState(ID3D12PipelineState* inPipelineState, ID3D12RootSignature* inRootSignature)
{
    if(inPipelineState)
    {
        m_CmdListHandle->SetPipelineState(inPipelineState);
    }
    if(inRootSignature)
    {
        m_CmdListHandle->SetGraphicsRootSignature(inRootSignature);
    }

    m_Context.CurrentSignature = inRootSignature;
    m_Context.CurrentPipelineState = inPipelineState;
    m_Device.GetDescriptorManager().BindShaderVisibleHeaps(m_CmdListHandle.Get());
}

void D3D12CommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer)
{
    if(IsValid())
    {
         uint32_t numRenderTargets = inFrameBuffer->GetNumRenderTargets();
         std::vector<RHIClearValue> clearColors(numRenderTargets);
         for(uint32_t i = 0; i < numRenderTargets; i++)
         {
             clearColors[i] = inFrameBuffer->GetRenderTarget(i)->GetClearValue();
         }
         SetFrameBuffer(inFrameBuffer, clearColors.data(), numRenderTargets);
    }
}

void D3D12CommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets)
{
    if(IsValid() && !m_IsClosed)
    {
         float depth = 1;
         uint8_t stencil = 0;
         if(inFrameBuffer->HasDepthStencil())
         {
             const RHIClearValue& clearValue = inFrameBuffer->GetDepthStencil()->GetClearValue();
             depth = clearValue.DepthStencil.Depth;
             stencil = clearValue.DepthStencil.Stencil;
         }
         SetFrameBuffer(inFrameBuffer, inColor, inNumRenderTargets, depth, stencil);
    }
}

void D3D12CommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets
    , float inDepth, uint8_t inStencil)
{
    if (IsValid() && !m_IsClosed)
    {
        FlushBarriers();
        const D3D12FrameBuffer* frameBuffer = CheckCast<D3D12FrameBuffer*>(inFrameBuffer.GetReference());
        if(frameBuffer && frameBuffer->IsValid() && inNumRenderTargets == frameBuffer->GetNumRenderTargets())
        {
            m_CmdListHandle->OMSetRenderTargets(frameBuffer->GetNumRenderTargets()
                , frameBuffer->GetRenderTargetViews()
                , false
                , frameBuffer->GetDepthStencilViews());
            for(uint32_t i = 0; i < inNumRenderTargets; ++i)
            {
                m_CmdListHandle->ClearRenderTargetView(frameBuffer->GetRenderTargetView(i), inColor[i].Color, 0, nullptr);
            }
            if(frameBuffer->HasDepthStencil())
            {
                D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
                const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(frameBuffer->GetDepthStencilFormat());
                if(formatInfo.HasStencil) flags |= D3D12_CLEAR_FLAG_STENCIL;
                m_CmdListHandle->ClearDepthStencilView(frameBuffer->GetDepthStencilView(), flags, inDepth, inStencil, 0, nullptr);
            }
        }
    }
}

void D3D12CommandList::SetViewports(const std::vector<RHIViewport>& inViewports)
{
    if(IsValid() && !IsClosed() && !inViewports.empty())
    {
        std::vector<D3D12_VIEWPORT> viewports(inViewports.size());
        for(uint32_t i = 0; i < inViewports.size(); ++i)
        {
            viewports[i].TopLeftX = inViewports[i].X;
            viewports[i].TopLeftY = inViewports[i].Y;
            viewports[i].Width = inViewports[i].Width;
            viewports[i].Height = inViewports[i].Height;
            viewports[i].MinDepth = inViewports[i].MinDepth;
            viewports[i].MaxDepth = inViewports[i].MaxDepth;
        }
        m_CmdListHandle->RSSetViewports((uint32_t) inViewports.size(), viewports.data());
    }
}

void D3D12CommandList::SetScissorRects(const std::vector<RHIRect>& inRects)
{
    if(IsValid() && !IsClosed() && !inRects.empty())
    {
        std::vector<D3D12_RECT> scissorRects(inRects.size());
        for(uint32_t i = 0; i < inRects.size(); ++i)
        {
            scissorRects[i].left = inRects[i].MinX;
            scissorRects[i].top = inRects[i].MinY;
            scissorRects[i].right = (long)inRects[i].MinX + inRects[i].Width;
            scissorRects[i].bottom = (long)inRects[i].MinY + inRects[i].Height;
        }
        m_CmdListHandle->RSSetScissorRects((uint32_t) inRects.size(), scissorRects.data());
    }
}

void D3D12CommandList::CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Buffer* buffer0 = CheckCast<D3D12Buffer*>(dstBuffer.GetReference());
        D3D12Buffer* buffer1 = CheckCast<D3D12Buffer*>(srcBuffer.GetReference());
    
        ID3D12Resource* dstRes = buffer0->GetBuffer();
        ID3D12Resource* srcRes = buffer1->GetBuffer();
        m_CmdListHandle->CopyBufferRegion(dstRes, dstOffset, srcRes, srcOffset, size);
        buffer0->ChangeState(D3D12_RESOURCE_STATE_COPY_DEST);
        buffer1->ChangeState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
}

void D3D12CommandList::CopyTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHITexture>& srcTexture)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Texture* t0 = CheckCast<D3D12Texture*>(dstTexture.GetReference());
        D3D12Texture* t1 = CheckCast<D3D12Texture*>(srcTexture.GetReference());
    
        ID3D12Resource* dstRes = t0->GetTexture();
        ID3D12Resource* srcRes = t1->GetTexture();
        m_CmdListHandle->CopyResource(dstRes, srcRes);
        t0->ChangeState(D3D12_RESOURCE_STATE_COPY_DEST);
        t1->ChangeState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
}

void D3D12CommandList::CopyTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHITexture>& srcTexture, const RHITextureSlice& srcSlice)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Texture* t0 = CheckCast<D3D12Texture*>(dstTexture.GetReference());
        D3D12Texture* t1 = CheckCast<D3D12Texture*>(srcTexture.GetReference());
    
        ID3D12Resource* dstRes = t0->GetTexture();
        ID3D12Resource* srcRes = t1->GetTexture();

        assert(dstSlice.Width == srcSlice.Width);
        assert(dstSlice.Height == srcSlice.Height);
        
        UINT dstSubresource = D3D12CalcSubresource(dstSlice.MipLevel, dstSlice.ArraySlice, 0, dstTexture->GetDesc().MipLevels, dstTexture->GetDesc().ArraySize);
        UINT srcSubresource = D3D12CalcSubresource(srcSlice.MipLevel, srcSlice.ArraySlice, 0, srcTexture->GetDesc().MipLevels, srcTexture->GetDesc().ArraySize);

        D3D12_TEXTURE_COPY_LOCATION dstLocation;
        dstLocation.pResource = dstRes;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = dstSubresource;

        D3D12_TEXTURE_COPY_LOCATION srcLocation;
        srcLocation.pResource = srcRes;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        srcLocation.SubresourceIndex = srcSubresource;

        D3D12_BOX srcBox;
        srcBox.left = srcSlice.X;
        srcBox.top = srcSlice.Y;
        srcBox.front = srcSlice.Z;
        srcBox.right = srcSlice.X + srcSlice.Width;
        srcBox.bottom = srcSlice.Y + srcSlice.Height;
        srcBox.back = srcSlice.Z + srcSlice.Depth;
        
        m_CmdListHandle->CopyTextureRegion(&dstLocation, dstSlice.X, dstSlice.Y, dstSlice.Z, &srcLocation, &srcBox);

        t0->ChangeState(D3D12_RESOURCE_STATE_COPY_DEST);
        t1->ChangeState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
}

void D3D12CommandList::CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHIBuffer>& srcBuffer)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        RHITextureSlice slice(dstTexture->GetDesc());
        CopyBufferToTexture(dstTexture, slice, srcBuffer, 0);
    }
}

void D3D12CommandList::CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Texture* t0 = CheckCast<D3D12Texture*>(dstTexture.GetReference());
        D3D12Buffer* t1 = CheckCast<D3D12Buffer*>(srcBuffer.GetReference());
    
        ID3D12Resource* dstRes = t0->GetTexture();
        ID3D12Resource* srcRes = t1->GetBuffer();

        UINT dstSubresource = D3D12CalcSubresource(dstSlice.MipLevel, dstSlice.ArraySlice, 0, dstTexture->GetDesc().MipLevels, dstTexture->GetDesc().ArraySize);
        D3D12_TEXTURE_COPY_LOCATION dstLocation;
        dstLocation.pResource = dstRes;
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = dstSubresource;

        D3D12_TEXTURE_COPY_LOCATION srcLocation;
        srcLocation.pResource = srcRes;
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint.Offset = srcOffset;
        srcLocation.PlacedFootprint.Footprint.Format = RHI::D3D12::ConvertFormat(dstTexture->GetDesc().Format);
        srcLocation.PlacedFootprint.Footprint.Width = dstSlice.Width;
        srcLocation.PlacedFootprint.Footprint.Height = dstSlice.Height;
        srcLocation.PlacedFootprint.Footprint.Depth = dstSlice.Depth;
        srcLocation.PlacedFootprint.Footprint.RowPitch = (uint32_t)dstSlice.Width * RHI::GetFormatInfo(dstTexture->GetDesc().Format).BytesPerBlock;
        
        m_CmdListHandle->CopyTextureRegion(&dstLocation, dstSlice.X, dstSlice.Y, dstSlice.Z, &srcLocation, nullptr);

        t0->ChangeState(D3D12_RESOURCE_STATE_COPY_DEST);
        t1->ChangeState(D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
}

void D3D12CommandList::SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer, size_t inOffset)
{
    if(IsValid() && !IsClosed())
    {
        const RHIBufferDesc& bufferDesc = inBuffer->GetDesc();
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.BufferLocation = inBuffer->GetGpuAddress() + inOffset;
        vbv.StrideInBytes = (uint32_t)bufferDesc.Stride;
        vbv.SizeInBytes = (uint32_t)bufferDesc.Size;
        m_CmdListHandle->IASetVertexBuffers(0, 1, &vbv);
    }
}

void D3D12CommandList::SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer, size_t inOffset)
{
    if(IsValid() && !IsClosed())
    {
        const RHIBufferDesc& bufferDesc = inBuffer->GetDesc();
        D3D12_INDEX_BUFFER_VIEW ibv{};
        ibv.SizeInBytes = (uint32_t)bufferDesc.Size;
        ibv.BufferLocation = inBuffer->GetGpuAddress() + inOffset;
        ibv.Format = RHI::D3D12::ConvertFormat(bufferDesc.Format);
        m_CmdListHandle->IASetIndexBuffer(&ibv);
    }
}
void D3D12CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t firstInstance)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        m_CmdListHandle->DrawInstanced(vertexCount, instanceCount, vertexOffset, firstInstance);
    }
}

void D3D12CommandList::DrawIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(indirectCommands.GetReference());
        ID3D12CommandSignature* signature = GetDrawCommandSignature();
        if(buffer && buffer->IsValid() && signature)
        {
            m_CmdListHandle->ExecuteIndirect(signature, drawCount, buffer->GetBuffer(), commandsBufferOffset, nullptr, 0);
        }
    }
}

void D3D12CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        m_CmdListHandle->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, (int)vertexOffset, firstInstance);
    }
}

void D3D12CommandList::DrawIndexedIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(indirectCommands.GetReference());
        ID3D12CommandSignature* signature = GetDrawIndexedCommandSignature();
        if(buffer && buffer->IsValid() && signature)
        {
            m_CmdListHandle->ExecuteIndirect(signature, drawCount, buffer->GetBuffer(), commandsBufferOffset, nullptr, 0);
        }
    }
}

void D3D12CommandList::Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        m_CmdListHandle->Dispatch(threadGroupX, threadGroupY, threadGroupZ);
    }
}

void D3D12CommandList::DispatchIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(indirectCommands.GetReference());
        ID3D12CommandSignature* signature = GetDrawIndexedCommandSignature();
        if(buffer && buffer->IsValid() && signature)
        {
            m_CmdListHandle->ExecuteIndirect(signature, count, buffer->GetBuffer(), commandsBufferOffset, nullptr, 0);
        }
    }
}

void D3D12CommandList::DispatchMesh(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        m_CmdListHandle->DispatchMesh(threadGroupX, threadGroupY, threadGroupZ);
    }
}

void D3D12CommandList::DispatchMeshIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(indirectCommands.GetReference());
        ID3D12CommandSignature* signature = GetDispatchMeshCommandSignature();
        if(buffer && buffer->IsValid() && signature)
        {
            m_CmdListHandle->ExecuteIndirect(signature, count, buffer->GetBuffer(), commandsBufferOffset, nullptr, 0);
        }
    }
}

void D3D12CommandList::DispatchRays(uint32_t width, uint32_t height, uint32_t depth, const RHIShaderTable& shaderTable)
{
    if(IsValid() && !IsClosed())
    {
        FlushBarriers();
        D3D12_DISPATCH_RAYS_DESC rayDesc;
        rayDesc.Width = width;
        rayDesc.Height = height;
        rayDesc.Depth = depth;
        rayDesc.RayGenerationShaderRecord.StartAddress = shaderTable.GetRayGenShaderEntry().StartAddress;
        rayDesc.RayGenerationShaderRecord.SizeInBytes = shaderTable.GetRayGenShaderEntry().SizeInBytes;
        rayDesc.HitGroupTable.StartAddress = shaderTable.GetHitGroupEntry().StartAddress;
        rayDesc.HitGroupTable.SizeInBytes = shaderTable.GetHitGroupEntry().SizeInBytes;
        rayDesc.HitGroupTable.StrideInBytes = shaderTable.GetHitGroupEntry().StrideInBytes;
        rayDesc.MissShaderTable.StartAddress = shaderTable.GetMissShaderEntry().StartAddress;
        rayDesc.MissShaderTable.SizeInBytes = shaderTable.GetMissShaderEntry().SizeInBytes;
        rayDesc.MissShaderTable.StrideInBytes = shaderTable.GetMissShaderEntry().StrideInBytes;
        rayDesc.CallableShaderTable.StartAddress = shaderTable.GetCallableShaderEntry().StartAddress;
        rayDesc.CallableShaderTable.SizeInBytes = shaderTable.GetCallableShaderEntry().SizeInBytes;
        rayDesc.CallableShaderTable.StrideInBytes = shaderTable.GetCallableShaderEntry().StrideInBytes;
        m_CmdListHandle->DispatchRays(&rayDesc);
    }
}

void D3D12CommandList::ResourceBarrier(RefCountPtr<RHITexture>& inResource , ERHIResourceStates inAfterState)
{
    if(IsValid() && !IsClosed())
    {
        D3D12Texture* texture = CheckCast<D3D12Texture*>(inResource.GetReference());
        if(texture && texture->IsValid())
        {
            D3D12_RESOURCE_STATES beforeState = texture->GetCurrentState();
            D3D12_RESOURCE_STATES afterState = RHI::D3D12::ConvertResourceStates(inAfterState);
            m_CachedBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTexture(), beforeState, afterState));
            texture->ChangeState(afterState);
        }
    }
}

void D3D12CommandList::ResourceBarrier(RefCountPtr<RHIBuffer>& inResource, ERHIResourceStates inAfterState)
{
    if(IsValid() && !IsClosed())
    {
        D3D12Buffer* buffer = CheckCast<D3D12Buffer*>(inResource.GetReference());
        if(buffer && buffer->IsValid())
        {
            D3D12_RESOURCE_STATES beforeState = buffer->GetCurrentState();
            D3D12_RESOURCE_STATES afterState  = RHI::D3D12::ConvertResourceStates(inAfterState);
            m_CachedBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(buffer->GetBuffer(), beforeState, afterState));
            buffer->ChangeState(afterState);
        }
    }
}

void D3D12CommandList::SetResourceSet(RefCountPtr<RHIResourceSet>& inResourceSet)
{
    if(IsValid() && !IsClosed())
    {
        D3D12ResourceSet* resourceSet = CheckCast<D3D12ResourceSet*>(inResourceSet.GetReference());
        if(resourceSet && inResourceSet->IsValid())
        {
            if(GetQueueType() == ERHICommandQueueType::Async)
            {
                resourceSet->SetComputeRootArguments(m_CmdListHandle.Get());
            }
            else
            {
                resourceSet->SetGraphicsRootArguments(m_CmdListHandle.Get());
            }
        }
    }
}

void D3D12CommandList::FlushBarriers()
{
    if(IsValid() && !m_CachedBarriers.empty())
    {
        m_CmdListHandle->ResourceBarrier((uint32_t)m_CachedBarriers.size(), m_CachedBarriers.data());
        m_CachedBarriers.clear();
    }
}

void D3D12CommandList::ShutdownInternal()
{
    m_CmdAllocatorHandle.Reset();
    m_CmdListHandle.Reset();
}

void D3D12CommandList::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring wname(m_Name.begin(), m_Name.end());
        m_CmdListHandle->SetName(wname.c_str());
    }
}

ID3D12CommandSignature* D3D12CommandList::GetDrawCommandSignature()
{
    if(m_DrawCommandSignature == nullptr)
    {
        CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, sizeof(RHIDrawArguments), m_DrawCommandSignature);
    }
    return m_DrawCommandSignature.Get();
}

ID3D12CommandSignature* D3D12CommandList::GetDrawIndexedCommandSignature()
{
    if(m_DrawIndexedCommandSignature == nullptr)
    {
        CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, sizeof(RHIDrawIndexedArguments), m_DrawIndexedCommandSignature);
    }
    return m_DrawIndexedCommandSignature.Get();
}

ID3D12CommandSignature* D3D12CommandList::GetDispatchCommandSignature()
{
    if(m_DispatchCommandSignature == nullptr)
    {
        if(!CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, sizeof(RHIDispatchArguments), m_DispatchCommandSignature))
        {
            return nullptr;
        }
    }
    return m_DispatchCommandSignature.Get();
}

ID3D12CommandSignature* D3D12CommandList::GetDispatchMeshCommandSignature()
{
    if(m_DispatchCommandSignature == nullptr)
    {
        if(!CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, sizeof(RHIDispatchArguments), m_DispatchMeshCommandSignature))
        {
            return nullptr;
        }
    }
    return m_DispatchCommandSignature.Get();
}

bool D3D12CommandList::CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE argument, uint32_t byteStride, Microsoft::WRL::ComPtr<ID3D12CommandSignature>& outCommandSignature)
{
    D3D12_INDIRECT_ARGUMENT_DESC argumentDescs{};
    argumentDescs.Type = argument;

    D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc{};
    commandSignatureDesc.ByteStride = byteStride;
    commandSignatureDesc.NumArgumentDescs = 1;
    commandSignatureDesc.pArgumentDescs = &argumentDescs;
    commandSignatureDesc.NodeMask = D3D12Device::GetNodeMask();

    HRESULT hr = m_Device.GetDevice()->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&outCommandSignature));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the command signature");
        return false;
    }
    
    return true;
}
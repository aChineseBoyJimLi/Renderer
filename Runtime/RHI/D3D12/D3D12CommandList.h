#pragma once

#include "../RHICommandList.h"
#include "D3D12Definitions.h"

struct D3D12CommandListContext
{
    ID3D12RootSignature* CurrentSignature = nullptr;
    ID3D12PipelineState* CurrentPipelineState = nullptr;

    void Clear()
    {
        CurrentSignature = nullptr;
        CurrentPipelineState = nullptr;
    }
};

class D3D12CommandList : public RHICommandList
{
public:
    ~D3D12CommandList() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void BeginMark(const char* name) override;
    void EndMark() override;
    void Begin() override;
    void End() override;
    void SetPipelineState(const RefCountPtr<RHIComputePipeline>& inPipelineState) override;
    void SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) override;
    void SetViewports(const std::vector<RHIViewport>& inViewports) override;
    void SetScissorRects(const std::vector<RHIRect>& inRects) override;

    void SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer, size_t inOffset = 0) override;
    void SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer, size_t inOffset = 0) override;
    
    void ResourceBarrier(RefCountPtr<RHITexture>& inResource , ERHIResourceStates inAfterState) override;
    void ResourceBarrier(RefCountPtr<RHIBuffer>& inResource, ERHIResourceStates inAfterState) override;
    void SetResourceSet(RefCountPtr<RHIResourceSet>& inResourceSet) override;
    
    void CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size) override;
    void CopyTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHITexture>& srcTexture) override;
    void CopyTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHITexture>& srcTexture, const RHITextureSlice& srcSlice) override;
    void CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHIBuffer>& srcBuffer) override;
    void CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset) override;

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
    void DrawIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
    void DrawIndexedIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset = 0) override;
    void Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
    void DispatchIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset = 0) override;
    void DispatchMesh(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
    void DispatchMeshIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset = 0) override;
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth, const RHIShaderTable& shaderTable) override;
    
    bool IsClosed() const override { return m_IsClosed; }
    ERHICommandQueueType GetQueueType() const override { return m_QueueType; }
    ID3D12GraphicsCommandList6* GetCommandList() const { return m_CmdListHandle.Get(); }
    
    
protected:
    void SetNameInternal() override;

private:
    friend class D3D12Device;
    D3D12CommandList(D3D12Device& inDevice, ERHICommandQueueType inType);
    void ShutdownInternal();
    void FlushBarriers();

    void SetPipelineState(ID3D12PipelineState* inPipelineState, ID3D12RootSignature* inRootSignature);
    ID3D12CommandSignature* GetDrawCommandSignature();
    ID3D12CommandSignature* GetDrawIndexedCommandSignature();
    ID3D12CommandSignature* GetDispatchCommandSignature();
    ID3D12CommandSignature* GetDispatchMeshCommandSignature();
    
    bool CreateCommandSignature(D3D12_INDIRECT_ARGUMENT_TYPE argument, uint32_t byteStride, Microsoft::WRL::ComPtr<ID3D12CommandSignature>& outCommandSignature);
    
    D3D12Device& m_Device;
    D3D12CommandListContext m_Context;
    const ERHICommandQueueType m_QueueType;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CmdAllocatorHandle;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_CmdListHandle;
    bool m_IsClosed;
    std::vector<CD3DX12_RESOURCE_BARRIER> m_CachedBarriers;
    std::vector<RefCountPtr<RHIBuffer>> m_StagingBuffers;

    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_DrawCommandSignature;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_DrawIndexedCommandSignature;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_DispatchCommandSignature;
    Microsoft::WRL::ComPtr<ID3D12CommandSignature> m_DispatchMeshCommandSignature;
};
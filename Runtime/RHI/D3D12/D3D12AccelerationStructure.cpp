#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"

RefCountPtr<RHIAccelerationStructure> D3D12Device::CreateBottomLevelAccelerationStructure(const std::vector<RHIRayTracingGeometryDesc>& inDesc)
{
    RefCountPtr<RHIAccelerationStructure> accelerationStructure(new D3D12AccelerationStructure(*this, inDesc));
    if(!accelerationStructure->Init())
    {
        Log::Error("[D3D12] failed to create acceleration structure");
    }
    return accelerationStructure;
}

RefCountPtr<RHIAccelerationStructure> D3D12Device::CreateTopLevelAccelerationStructure(const std::vector<RHIRayTracingInstanceDesc>& inDesc)
{
    RefCountPtr<RHIAccelerationStructure> accelerationStructure(new D3D12AccelerationStructure(*this, inDesc));
    if(!accelerationStructure->Init())
    {
        Log::Error("[D3D12] failed to create acceleration structure");
    }
    return accelerationStructure;
}

D3D12AccelerationStructure::D3D12AccelerationStructure(D3D12Device& inDevice, const std::vector<RHIRayTracingGeometryDesc>& inDesc)
    : m_Device(inDevice)
    , m_GeometryDesc(inDesc)
    , m_IsTopLevel(false)
    , m_IsBuilt(false)
    , m_InstanceBuffer(nullptr)
    , m_AccelerationStructureBuffer(nullptr)
    , m_BuildInfos{}
    , m_PrebuildInfo{}
{
    
}

D3D12AccelerationStructure::D3D12AccelerationStructure(D3D12Device& inDevice, const std::vector<RHIRayTracingInstanceDesc>& inDesc)
    : m_Device(inDevice)
    , m_InstanceDesc(inDesc)
    , m_IsTopLevel(true)
    , m_IsBuilt(false)
    , m_InstanceBuffer(nullptr)
    , m_AccelerationStructureBuffer(nullptr)
    , m_BuildInfos{}
    , m_PrebuildInfo{}
{
    
}

D3D12AccelerationStructure::~D3D12AccelerationStructure()
{
    ShutdownInternal();
}

bool D3D12AccelerationStructure::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Acceleration structure already initialized.");
        return true;
    }

    return m_IsTopLevel ? InitTopLevel() : InitBottomLevel();
}

bool D3D12AccelerationStructure::InitBottomLevel()
{
    D3D12_RAYTRACING_GEOMETRY_DESC accelerationStructureGeometry{};
    for(uint32_t i = 0; i < m_GeometryDesc.size(); ++i)
    {
        const auto& geometryDesc = m_GeometryDesc[i];
        accelerationStructureGeometry.Flags = RHI::D3D12::ConvertGeometryFlags(geometryDesc.Flags);
        if(geometryDesc.Type == ERHIRayTracingGeometryType::Triangles)
        {
            RHIBuffer* vertexBuffer = geometryDesc.Triangles.VertexBuffer;
            RHIBuffer* indexBuffer = geometryDesc.Triangles.IndexBuffer;
            const RHIBufferDesc& vbDesc = vertexBuffer->GetDesc();
            if(vertexBuffer == nullptr || !vertexBuffer->IsValid() || indexBuffer == nullptr || !indexBuffer->IsValid())
            {
                continue;
            }
            accelerationStructureGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            accelerationStructureGeometry.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGpuAddress();
            accelerationStructureGeometry.Triangles.VertexBuffer.StrideInBytes = vbDesc.Stride;
            accelerationStructureGeometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
            accelerationStructureGeometry.Triangles.VertexCount = geometryDesc.Triangles.VertexCount;
            accelerationStructureGeometry.Triangles.IndexBuffer = indexBuffer->GetGpuAddress();
            accelerationStructureGeometry.Triangles.IndexCount = geometryDesc.Triangles.IndexCount;
            accelerationStructureGeometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
            accelerationStructureGeometry.Triangles.Transform3x4 = 0;
        }
        else
        {
            RHIBuffer* aabbBuffer = geometryDesc.AABBs.Buffer;
            const RHIBufferDesc& aabbDesc = aabbBuffer->GetDesc();
            if(aabbBuffer == nullptr || !aabbBuffer->IsValid())
            {
                continue;
            }
            accelerationStructureGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
            accelerationStructureGeometry.AABBs.AABBCount = 1;
            accelerationStructureGeometry.AABBs.AABBs.StartAddress = aabbBuffer->GetGpuAddress();
            accelerationStructureGeometry.AABBs.AABBs.StrideInBytes = aabbDesc.Stride != 0 ? aabbDesc.Stride : sizeof(RHIAABB);
        }
        m_GeometryDescD3D.push_back(accelerationStructureGeometry);
    }

    m_BuildInfos.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    m_BuildInfos.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    m_BuildInfos.pGeometryDescs = m_GeometryDescD3D.data();
    m_BuildInfos.NumDescs = (uint32_t) m_GeometryDescD3D.size();
    m_BuildInfos.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    
    m_Device.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&m_BuildInfos, &m_PrebuildInfo);
    
    RHIBufferDesc asDesc = RHIBufferDesc::AccelerationStructure(m_PrebuildInfo.ResultDataMaxSizeInBytes);
    m_AccelerationStructureBuffer = m_Device.CreateD3D12Buffer(asDesc);

    if(!m_AccelerationStructureBuffer.GetReference() || !m_AccelerationStructureBuffer->IsValid())
    {
        Log::Error("[D3D12] Failed to create bottom level acceleration structure");
        return false;
    }
    
    return true;
}

bool D3D12AccelerationStructure::InitTopLevel()
{
    RHIBufferDesc instanceBufferDesc;
    instanceBufferDesc.CpuAccess = ERHICpuAccessMode::Write;
    instanceBufferDesc.Usages = ERHIBufferUsage::AccelerationStructureBuildInput;
    instanceBufferDesc.Size = sizeof(RHIRayTracingInstanceDesc) * m_InstanceDesc.size();
    instanceBufferDesc.Stride = sizeof(RHIRayTracingInstanceDesc);

    m_InstanceBuffer = m_Device.CreateD3D12Buffer(instanceBufferDesc);

    if(m_InstanceBuffer.GetReference() == nullptr || !m_InstanceBuffer.IsValid())
    {
        Log::Error("[D3D12] failed to create top level instance buffer");
        return false;
    }

    m_InstanceBuffer->WriteData(m_InstanceDesc.data(), instanceBufferDesc.Size);


    m_BuildInfos.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    m_BuildInfos.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    m_BuildInfos.InstanceDescs = m_InstanceBuffer->GetGpuAddress();
    m_BuildInfos.NumDescs = (uint32_t)m_InstanceDesc.size();
    m_BuildInfos.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    
    m_Device.GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&m_BuildInfos, &m_PrebuildInfo);

    RHIBufferDesc asDesc = RHIBufferDesc::AccelerationStructure(m_PrebuildInfo.ResultDataMaxSizeInBytes);
    m_AccelerationStructureBuffer = m_Device.CreateD3D12Buffer(asDesc);

    if(!m_AccelerationStructureBuffer.GetReference() || !m_AccelerationStructureBuffer->IsValid())
    {
        Log::Error("[D3D12] Failed to create bottom level acceleration structure");
        return false;
    }
    
    return true;
}


bool D3D12AccelerationStructure::IsValid() const
{
    return m_AccelerationStructureBuffer.GetReference() != nullptr && m_AccelerationStructureBuffer->IsValid();
}

void D3D12AccelerationStructure::Shutdown()
{
    ShutdownInternal();
}

void D3D12AccelerationStructure::ShutdownInternal()
{
    m_AccelerationStructureBuffer.SafeRelease();
    m_InstanceBuffer.SafeRelease();
    m_GeometryDescD3D.clear();
}

RefCountPtr<RHIBuffer> D3D12AccelerationStructure::GetScratchBuffer() const
{
    if(IsValid())
    {
        RHIBufferDesc scratchBufferDesc;
        scratchBufferDesc.Size = m_PrebuildInfo.ScratchDataSizeInBytes;
        scratchBufferDesc.Usages = ERHIBufferUsage::UnorderedAccess;
        scratchBufferDesc.CpuAccess = ERHICpuAccessMode::None;
        return m_Device.CreateBuffer(scratchBufferDesc);
    }
    return nullptr;
}

void D3D12AccelerationStructure::Build(ID3D12GraphicsCommandList* inCmdList, RefCountPtr<RHIBuffer>& inScratchBuffer)
{
    if(IsValid() && !IsBuilt() && inScratchBuffer.GetReference() && inScratchBuffer->IsValid())
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc{};
        buildDesc.Inputs = m_BuildInfos;
        buildDesc.ScratchAccelerationStructureData = inScratchBuffer->GetGpuAddress();
        buildDesc.DestAccelerationStructureData = m_AccelerationStructureBuffer->GetGpuAddress();

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> cmdList;
        HRESULT hr = inCmdList->QueryInterface(cmdList.GetAddressOf());
        if(SUCCEEDED(hr))
        {
            cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
        }
    }
}

void D3D12AccelerationStructure::SetNameInternal()
{
    m_AccelerationStructureBuffer->SetName(m_Name);
}
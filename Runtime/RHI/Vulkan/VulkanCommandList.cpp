#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"

std::shared_ptr<RHICommandList> VulkanDevice::CreateCommandList(ERHICommandQueueType inType)
{
    std::shared_ptr<RHICommandList> commandList(new VulkanCommandList(*this, inType));
    if(!commandList->Init())
    {
        Log::Error("[Vulkan] Failed to create a command list");
    }
    return commandList;
}

VulkanCommandList::VulkanCommandList(VulkanDevice& inDevice, ERHICommandQueueType inType)
    : m_Device(inDevice)
    , m_QueueType(inType)
    , m_CmdPoolHandle(VK_NULL_HANDLE)
    , m_CmdBufferHandle(VK_NULL_HANDLE)
    , m_IsClosed(true)
{
    
}

VulkanCommandList::~VulkanCommandList()
{
    ShutdownInternal();
}

bool VulkanCommandList::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Command list is already initialized");
        return true;
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_Device.GetQueueFamilyIndex(m_QueueType);
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(m_Device.GetDevice(), &poolInfo, nullptr, &m_CmdPoolHandle);

    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CmdPoolHandle;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CmdBufferHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    return true;
}

void VulkanCommandList::Shutdown()
{
    ShutdownInternal();
}

void VulkanCommandList::Begin()
{
    if (IsValid() && m_IsClosed)
    {
        vkResetCommandPool(m_Device.GetDevice(), m_CmdPoolHandle, 0);
        vkResetCommandBuffer(m_CmdBufferHandle, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_CmdBufferHandle, &beginInfo);
        m_IsClosed = false;
    }
}

void VulkanCommandList::End()
{
    if (IsValid() && !m_IsClosed)
    {
        vkEndCommandBuffer(m_CmdBufferHandle);
        m_IsClosed = true;
    }
}

bool VulkanCommandList::IsValid() const
{
    bool valid = m_CmdPoolHandle != VK_NULL_HANDLE && m_CmdBufferHandle != VK_NULL_HANDLE;
    return valid;
}

void VulkanCommandList::ShutdownInternal()
{
    if(m_CmdBufferHandle != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_Device.GetDevice(), m_CmdPoolHandle, 1, &m_CmdBufferHandle);
        m_CmdBufferHandle = VK_NULL_HANDLE;
    }

    if(m_CmdPoolHandle != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device.GetDevice(), m_CmdPoolHandle, nullptr);
        m_CmdPoolHandle = VK_NULL_HANDLE;
    }
}

void VulkanCommandList::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(m_CmdBufferHandle), m_Name);
    }
}
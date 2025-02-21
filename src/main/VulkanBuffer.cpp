#include "VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(VulkanWindow* vulkanWindow, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex) 
    : m_vulkanWindow(vulkanWindow)
{
    m_device = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_device);
    
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    m_result = m_deviceFunctions->vkCreateBuffer(m_device, &bufferCreateInfo, nullptr, &m_buffer);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create buffer (error code: %d)", m_result);
        return;
    }

    VkMemoryRequirements memoryRequirements;
    m_deviceFunctions->vkGetBufferMemoryRequirements(m_device, m_buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex
    };

    m_result = m_deviceFunctions->vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &m_memory);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to allocate memory (error code: %d)", m_result);
        return;
    }

    m_result = m_deviceFunctions->vkBindBufferMemory(m_device, m_buffer, m_memory, 0);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to bind buffer memory (error code: %d)", m_result);
        return; 
    }
}

VulkanBuffer::~VulkanBuffer() 
{
    cleanup();
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept 
{
    *this = std::move(other);
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept 
{
    if (this != &other) {
        cleanup();
        m_vulkanWindow = other.m_vulkanWindow;
        m_buffer = other.m_buffer;
        m_memory = other.m_memory;

        other.m_vulkanWindow = VK_NULL_HANDLE;
        other.m_buffer = VK_NULL_HANDLE;
        other.m_memory = VK_NULL_HANDLE;
    }
    return *this;
}

void VulkanBuffer::copyData(const void* data, VkDeviceSize size) 
{
    void* mappedData = nullptr;
    m_result = m_deviceFunctions->vkMapMemory(m_device, m_memory, 0, size, 0, &mappedData);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to map memory (error code: %d)", m_result);
        return;
    }

    memcpy(mappedData, data, static_cast<size_t>(size));
    m_deviceFunctions->vkUnmapMemory(m_device, m_memory);
}

void VulkanBuffer::cleanup()
{
    if (m_buffer) vkDestroyBuffer(m_device, m_buffer, nullptr);
    if (m_memory) vkFreeMemory(m_device, m_memory, nullptr);
}

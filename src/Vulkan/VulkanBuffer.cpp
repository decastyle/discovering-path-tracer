#include "VulkanBuffer.h"

VulkanBuffer::VulkanBuffer(VulkanWindow* vulkanWindow, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex) 
    : m_vulkanWindow(vulkanWindow), 
      m_size(size), 
      m_usage(usage), 
      m_memoryTypeIndex(memoryTypeIndex)
{
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_vulkanWindow->device());

    createBuffer();
    allocateMemory();
}

VulkanBuffer::~VulkanBuffer() 
{
    cleanup();
}

void VulkanBuffer::swap(VulkanBuffer& other) noexcept
{
    std::swap(m_vulkanWindow, other.m_vulkanWindow);
    std::swap(m_size, other.m_size);
    std::swap(m_usage, other.m_usage);
    std::swap(m_memoryTypeIndex, other.m_memoryTypeIndex);
    
    // Vulkan resources
    std::swap(m_buffer, other.m_buffer);
    std::swap(m_memory, other.m_memory);
    
    // Device resources
    std::swap(m_result, other.m_result);
    std::swap(m_deviceFunctions, other.m_deviceFunctions);
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept 
{
    if (this != &other) 
    {
        cleanup();
        swap(other);
    }
    return *this;
}

void VulkanBuffer::createBuffer()
{
    VkBufferCreateInfo bufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = m_size,
        .usage = m_usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    m_result = m_deviceFunctions->vkCreateBuffer(m_vulkanWindow->device(), &bufferCreateInfo, nullptr, &m_buffer);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create buffer (error code: %d)", m_result);
        return;
    }
}

void VulkanBuffer::allocateMemory()
{
    VkMemoryRequirements memoryRequirements;
    m_deviceFunctions->vkGetBufferMemoryRequirements(m_vulkanWindow->device(), m_buffer, &memoryRequirements);

    VkMemoryAllocateInfo memoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = m_memoryTypeIndex
    };

    m_result = m_deviceFunctions->vkAllocateMemory(m_vulkanWindow->device(), &memoryAllocateInfo, nullptr, &m_memory);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to allocate memory (error code: %d)", m_result);
        return;
    }

    m_result = m_deviceFunctions->vkBindBufferMemory(m_vulkanWindow->device(), m_buffer, m_memory, 0);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to bind buffer memory (error code: %d)", m_result);
        return; 
    }
}

void VulkanBuffer::cleanup()
{
    if (m_memory) 
    {
        vkFreeMemory(m_vulkanWindow->device(), m_memory, nullptr);
        m_memory = VK_NULL_HANDLE;
    }
    if (m_buffer) 
    {
        vkDestroyBuffer(m_vulkanWindow->device(), m_buffer, nullptr);
        m_buffer = VK_NULL_HANDLE;
    }
}

void VulkanBuffer::copyData(const void* data, VkDeviceSize size, VkDeviceSize offset)
{
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_vulkanWindow->physicalDevice(), &physicalDeviceMemoryProperties);

    // Ensure the memory type is host-visible
    if (!(physicalDeviceMemoryProperties.memoryTypes[m_memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
    {
        qWarning("Cannot map memory: Buffer is not host-visible.");
        return;
    }
    
    void* mappedData = nullptr;
    m_result = m_deviceFunctions->vkMapMemory(m_vulkanWindow->device(), m_memory, offset, size, 0, &mappedData);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to map memory (error code: %d)", m_result);
        return;
    }

    memcpy(mappedData, data, static_cast<size_t>(size));

    // Flush memory if not HOST_COHERENT
    if (!(physicalDeviceMemoryProperties.memoryTypes[m_memoryTypeIndex].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        VkMappedMemoryRange range{};
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory = m_memory;
        range.offset = offset;
        range.size = size;
        vkFlushMappedMemoryRanges(m_vulkanWindow->device(), 1, &range);
    }

    m_deviceFunctions->vkUnmapMemory(m_vulkanWindow->device(), m_memory);
}
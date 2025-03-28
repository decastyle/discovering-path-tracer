#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanDeviceFunctions>

class VulkanWindow;

class VulkanBuffer 
{
public:
    VulkanBuffer() = default;
    VulkanBuffer(VulkanWindow* vulkanWindow, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex);
    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    VkBuffer getBuffer() const { return m_buffer; }
    
    void copyData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

    void destroy() { cleanup(); }

private:
    void createBuffer();
    void allocateMemory();
    void cleanup();
    void swap(VulkanBuffer& other) noexcept;

    VulkanWindow* m_vulkanWindow = nullptr;
    VkDeviceSize m_size{};
    VkBufferUsageFlags m_usage{};
    uint32_t m_memoryTypeIndex{};

    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    
    VkResult m_result = VK_NOT_READY;
    QVulkanDeviceFunctions* m_deviceFunctions = nullptr;
};
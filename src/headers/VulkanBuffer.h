#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <cstring>
#include <QVulkanFunctions>
#include "VulkanWindow.h"

class VulkanBuffer 
{
public:
    VulkanBuffer(VulkanWindow* vulkanWindow, VkDeviceSize size, VkBufferUsageFlags usage, uint32_t memoryTypeIndex);

    ~VulkanBuffer();

    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;

    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

    VkBuffer get() const { return m_buffer; }
    void copyData(const void* data, VkDeviceSize size);

private:
    void cleanup();

    VulkanWindow *m_vulkanWindow;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;

    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result{};
    QVulkanDeviceFunctions *m_deviceFunctions = VK_NULL_HANDLE;
};

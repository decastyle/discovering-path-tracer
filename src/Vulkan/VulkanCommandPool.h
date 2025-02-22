#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanFunctions>
#include "VulkanWindow.h"

class VulkanWindow;

class VulkanCommandPool
{
public:
    VulkanCommandPool() = default; 
    VulkanCommandPool(VulkanWindow* vulkanWindow, uint32_t queueFamilyIndex);
    ~VulkanCommandPool();

    VulkanCommandPool& operator=(VulkanCommandPool&& other) noexcept;

    VkCommandPool getCommandPool() const { return m_commandPool; }

    void destroy() { cleanup(); }

private:
    void createCommandPool();
    void cleanup();

    void swap(VulkanCommandPool& other) noexcept;

    VulkanWindow* m_vulkanWindow = nullptr;
    uint32_t m_queueFamilyIndex{};

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    
    VkResult m_result{};
    QVulkanDeviceFunctions *m_deviceFunctions = VK_NULL_HANDLE;
};

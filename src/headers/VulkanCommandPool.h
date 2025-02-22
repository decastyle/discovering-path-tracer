#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanFunctions>
#include "VulkanWindow.h"

class VulkanCommandPool
{
public:
    VulkanCommandPool(QVulkanWindow* vulkanWindow, uint32_t queueFamilyIndex);
    ~VulkanCommandPool();

private:
    void createCommandPool();
    void cleanup();

    QVulkanWindow* m_vulkanWindow = nullptr;
    uint32_t m_queueFamilyIndex{};

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result{};
    QVulkanDeviceFunctions *m_deviceFunctions = VK_NULL_HANDLE;
};

#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanFunctions>
#include "VulkanWindow.h"

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer(QVulkanWindow* vulkanWindow, VkCommandPool commandPool, VkQueue queue);
    ~VulkanCommandBuffer();

    void beginSingleTimeCommandBuffer();
    void endSubmitAndWait();

private:
    void createCommandBuffer();
    void cleanup();

    QVulkanWindow* m_vulkanWindow = nullptr;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;

    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    bool m_isRecording = false;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result{};
    QVulkanDeviceFunctions *m_deviceFunctions = VK_NULL_HANDLE;
};

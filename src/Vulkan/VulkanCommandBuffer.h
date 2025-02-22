#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanFunctions>
#include "VulkanWindow.h"

class VulkanWindow;

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer() = default; 
    VulkanCommandBuffer(VulkanWindow* vulkanWindow, VkCommandPool commandPool, VkQueue queue);
    ~VulkanCommandBuffer();

    void beginSingleTimeCommandBuffer();
    void endSubmitAndWait();

    VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }

private:
    void createCommandBuffer();
    void cleanup();

    VulkanWindow* m_vulkanWindow = nullptr;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;

    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    bool m_isRecording = false;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result{};
    QVulkanDeviceFunctions *m_deviceFunctions = VK_NULL_HANDLE;
};

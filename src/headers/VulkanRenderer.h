#pragma once

#include <QObject>
#include <QVulkanWindow>
#include <QElapsedTimer>
#include <vulkan/vulkan.h>
#include "VulkanWindow.h"

class VulkanWindow;

class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(VulkanWindow *vulkanWindow);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

protected:
    VulkanWindow *m_vulkanWindow;
    QVulkanDeviceFunctions *m_deviceFunctions;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;

    VkBuffer m_vertexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexStagingMemory = VK_NULL_HANDLE;

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformMemory = VK_NULL_HANDLE;

    VkImage m_renderImage = VK_NULL_HANDLE;
    VkDeviceMemory m_renderImageMemory = VK_NULL_HANDLE;
    VkImageView m_renderImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    VkDescriptorBufferInfo m_uniformBufferInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    // VkDescriptorImageInfo m_descriptorImageInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;

    QElapsedTimer m_renderTimer;
};

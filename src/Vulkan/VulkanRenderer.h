#pragma once

#include <QVulkanWindowRenderer>
#include <QElapsedTimer>
#include <QVulkanDeviceFunctions>

#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanCommandPool.h"
#include "Camera.h"


class VulkanWindow;

class VulkanRenderer : public QVulkanWindowRenderer 
{
public:
    VulkanRenderer(VulkanWindow* vulkanWindow);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

    qint64 getDeltaTimeNs() { return m_renderTimeNs; }

    void copyStorageImage(VkSemaphore semaphore);

protected:
    VulkanWindow* m_vulkanWindow = nullptr;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    VkDescriptorBufferInfo m_uniformBufferInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VulkanCommandPool m_graphicsCommandPool{};

    uint32_t m_graphicsQueueFamilyIndex{};
    uint32_t m_computeQueueFamilyIndex{};

    QElapsedTimer m_renderTimer{};
    qint64 m_renderTimeNs{};

    VulkanBuffer m_vertexBuffer{};
    VulkanBuffer m_vertexStagingBuffer{};
    VulkanBuffer m_uniformBuffer{};
    VulkanImage m_renderImage{};

    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result = VK_NOT_READY;
    QVulkanDeviceFunctions* m_deviceFunctions = nullptr;
};
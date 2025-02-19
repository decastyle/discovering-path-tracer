#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QObject>
#include <QVulkanWindow>
#include <QElapsedTimer>
#include <vulkan/vulkan.h>

#include "VulkanWindow.h"

class VulkanWindow;
class VulkanRenderer;


class VulkanRendererHelper : public QObject
{
    Q_OBJECT
public:
    VulkanRendererHelper(VulkanRenderer *renderer) : m_renderer(renderer) {}

signals:
    void updateSwapChain();
    void deviceReady();

private:
    VulkanRenderer *m_renderer;
};


class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(VulkanWindow *w);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;
    void startNextFrame() override;

    VulkanRendererHelper *m_helper;

protected:
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;

    VkBuffer m_vertexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexStagingMemory = VK_NULL_HANDLE;

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufferInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    VkDescriptorImageInfo descImageInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkImage m_renderImage = VK_NULL_HANDLE;
    VkDeviceMemory m_renderImageMemory = VK_NULL_HANDLE;
    VkImageView m_renderImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler;

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue;

    VkCommandPool m_cmdPool;

    QElapsedTimer m_renderTimer;
    float m_renderTimeNs = 0.0f;
    float m_fps = 0.0f;
};

#endif // VULKANRENDERER_H
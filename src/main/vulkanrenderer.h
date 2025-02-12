#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QObject>
#include <QVulkanWindow>
#include <QElapsedTimer>
#include "vulkanwindow.h"

class VulkanWindow;

class VulkanRendererHelper : public QObject
{
    Q_OBJECT

signals:
    void updateSwapChain();
    void deviceReady();
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
    VkShaderModule createShaderModule(const QString &filename);

    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;

    VkBuffer m_stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingMemory = VK_NULL_HANDLE;

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformMemory = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufferInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    QElapsedTimer m_renderTimer;
    float m_renderTimeNs = 0.0f;
    float m_fps = 0.0f;
};

#endif // VULKANRENDERER_H
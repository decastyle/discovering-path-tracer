#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QObject>
#include <QVulkanWindow>
#include "vulkanwindow.h"

class VulkanWindow;

class VulkanRendererHelper : public QObject
{
    Q_OBJECT

signals:
    void updateSwapChain();
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

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
    VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    QMatrix4x4 m_proj;
};

#endif // VULKANRENDERER_H
#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QObject>
#include <QVulkanWindow>
#include <QElapsedTimer>
#include "vulkanwindow.h"
#include <vulkan/vulkan.h>
#include <mutex>

class VulkanWindow;
class VulkanRenderer;

class VulkanRendererHelper : public QObject
{
    Q_OBJECT

public:
    VulkanRendererHelper(VulkanRenderer *renderer) : m_renderer(renderer) {}

signals:
    void updateSwapChain();

public slots:
    void onCopySampledImageHelper();

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

    void onCopySampledImage();

    // std::mutex *getQueueMutex()
    // {
    //     return &queueMutex;
    // }

    VulkanRendererHelper *m_helper;

protected:
    

    // std::mutex queueMutex;

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VkShaderModule createShaderModule(const QString &filename);

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

    VkImage m_stagingImage = VK_NULL_HANDLE;
    VkDeviceMemory m_stagingImageMemory = VK_NULL_HANDLE;
    VkImageView m_stagingImageView = VK_NULL_HANDLE;

    VkImage m_renderImage[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    VkDeviceMemory m_renderImageMemory[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];
    VkImageView m_renderImageView[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

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
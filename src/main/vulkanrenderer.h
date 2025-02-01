#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindow>
#include "vulkanwindow.h"

class VulkanRenderer : public QVulkanWindowRenderer
{

public:
    VulkanRenderer(VulkanWindow *w);
    void initResources() override;

    void initSwapChainResources() override;
    void releaseSwapChainResources() override;

    void releaseResources() override;

    void startNextFrame() override;

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

    QMatrix4x4 m_proj;          // TODO: Create camera Q_OBJECT and link camera parameters (translate, FOV, rotation) to m_proj
    // Connect mouse wheel to zoom, GUI scroll to FOV, and rotate camera only when Alt is pressed
    // m_proj is used to drive rasterization view
    // In case of raytracing, camera parameters will be used to define ray's spawn point

    float m_rotation = 0.0f;    // Not used
    float m_hue = 0;            // Not used

};

#endif // VULKANRENDERER_H
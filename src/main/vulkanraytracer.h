#ifndef VULKANRAYTRACER_H
#define VULKANRAYTRACER_H

#include <vulkan/vulkan.h>
#include <QObject>
#include <QString>
#include <QWindow>
#include <QCursor>
#include <QFile>
#include <QVulkanWindow>
#include <QVulkanFunctions>
#include "vulkanwindow.h"


class VulkanWindow;

class VulkanRayTracer : public QObject
{
    Q_OBJECT

public:
    VulkanRayTracer(VulkanWindow *w);
    VkImage getStorageImage();

public slots:
    void onDeviceReady(); // TODO: Find a better way to initialize device (right now ray-tracer waits for renderer to initialize it through QVulkanWindow)

private:
    void initComputePipeline();

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    VkShaderModule createShaderModule(const QString &filename);
    VkQueue m_computeQueue;
    
    VkImage m_storageImage;
    VkDeviceMemory m_storageImageMemory;
    VkImageView m_storageImageView;

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet = VK_NULL_HANDLE;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;
};


#endif // VULKANRAYTRACER_H
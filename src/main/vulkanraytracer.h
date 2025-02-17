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

signals:
    void copySampledImage();

public slots:
    void onDeviceReady(); // TODO: Find a better way to initialize device (right now ray-tracer waits for renderer to initialize it through QVulkanWindow)

private:
    std::mutex *queueMutex;
    void initRayTracing();

    QVulkanInstance* m_instance;

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    VkShaderModule createShaderModule(const QString &filename);
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    
    VkImage m_storageImage = VK_NULL_HANDLE;
    VkDeviceMemory m_storageImageMemory = VK_NULL_HANDLE;
    VkImageView m_storageImageView = VK_NULL_HANDLE;

    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    VkBuffer m_vertexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexStagingMemory = VK_NULL_HANDLE;

    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
    VkBuffer m_indexStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexStagingMemory = VK_NULL_HANDLE;

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet = VK_NULL_HANDLE;

    VkPipelineCache m_raytracingPipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_raytracingPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_raytracingPipeline = VK_NULL_HANDLE;
};


#endif // VULKANRAYTRACER_H
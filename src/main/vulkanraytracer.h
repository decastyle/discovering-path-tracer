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
    VulkanRayTracer(QVulkanInstance* instance, VulkanWindow *w);
    VkImage getStorageImage();

public slots:
    void onDeviceReady(); // TODO: Find a better way to initialize device (right now ray-tracer waits for renderer to initialize it through QVulkanWindow)

private:
    void initComputePipeline();

    QVulkanInstance* m_instance;
    VkDevice dev;

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    VkShaderModule createShaderModule(const QString &filename);
    VkQueue m_computeQueue = VK_NULL_HANDLE;
    
    VkImage m_storageImage = VK_NULL_HANDLE;
    VkDeviceMemory m_storageImageMemory = VK_NULL_HANDLE;
    VkImageView m_storageImageView = VK_NULL_HANDLE;
    VkBuffer m_storageStagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_storageStagingMemory = VK_NULL_HANDLE;

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

    VkPipelineCache m_computePipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;
};


#endif // VULKANRAYTRACER_H
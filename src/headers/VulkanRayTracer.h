#pragma once

#include <vulkan/vulkan.h>
#include <QObject>
#include <QString>
#include <QWindow>
#include <QCursor>
#include <QFile>
#include <QVulkanWindow>
#include <QVulkanFunctions>
#include <vector>

#include "VulkanWindow.h"

class VulkanWindow;

class VulkanRayTracer 
{
public:
    VulkanRayTracer(VulkanWindow *w);

    VkImage getStorageImage();

    void deviceReady();

private:
    void initComputePipeline();  

    VulkanWindow *m_vulkanWindow;
    
    VkImage m_storageImage = VK_NULL_HANDLE;
    VkDeviceMemory m_storageImageMemory = VK_NULL_HANDLE;
    VkImageView m_storageImageView = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;

    VkQueue m_computeQueue = VK_NULL_HANDLE;

    QVulkanDeviceFunctions *m_deviceFunctions;
};

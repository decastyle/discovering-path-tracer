#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanDeviceFunctions>
#include <QElapsedTimer>

#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"

class VulkanWindow;

class VulkanRayTracer 
{
public:
    VulkanRayTracer(VulkanWindow* w);

    VkImage getStorageImage() { return m_storageImage.getImage(); }
    void initRayTracer();

private:
    void initComputePipeline();
    void mainLoop();

    VulkanWindow* m_vulkanWindow = nullptr;

    VulkanBuffer m_vertexBuffer{};
    VulkanBuffer m_vertexStagingBuffer{};

    VulkanBuffer m_indexBuffer{};
    VulkanBuffer m_indexStagingBuffer{};

    VulkanBuffer m_BVHBuffer{};
    VulkanBuffer m_BVHStagingBuffer{};

    VulkanBuffer m_uniformBuffer{};

    VulkanImage m_storageImage{};
    
    // VkImage m_storageImage = VK_NULL_HANDLE;
    // VkDeviceMemory m_storageImageMemory = VK_NULL_HANDLE;
    // VkImageView m_storageImageView = VK_NULL_HANDLE;

    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;

    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_computeQueue = VK_NULL_HANDLE;

    uint32_t m_graphicsQueueFamilyIndex{};
    uint32_t m_computeQueueFamilyIndex{};

    VulkanCommandPool m_computeCommandPool{};

    QElapsedTimer m_rayTraceTimer{};
    qint64 m_rayTraceTimeNs{};
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result = VK_NOT_READY;
    QVulkanDeviceFunctions* m_deviceFunctions = nullptr;
};
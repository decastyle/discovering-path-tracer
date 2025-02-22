#pragma once

#include <vulkan/vulkan.h>
#include <QDebug>
#include <QVulkanFunctions>
#include "VulkanWindow.h"

class VulkanImage
{
public:
    VulkanImage(QVulkanWindow* vulkanWindow, uint32_t width, uint32_t height, VkBufferUsageFlags usage, uint32_t memoryTypeIndex);
    ~VulkanImage();

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    VkImage getImage() const { return m_image; }
    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler; }

private:
    void createImage();
    void allocateMemory();
    void createImageView();
    void createSampler();
    void cleanup();

    QVulkanWindow* m_vulkanWindow = nullptr;
    uint32_t m_width{};
    uint32_t m_height{};
    VkBufferUsageFlags m_usage{};
    uint32_t m_memoryTypeIndex{};

    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkResult m_result{};
    QVulkanDeviceFunctions *m_deviceFunctions = VK_NULL_HANDLE;
};

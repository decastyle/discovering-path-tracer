#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanDeviceFunctions>

class VulkanWindow;

class VulkanImage
{
public:
    VulkanImage() = default;
    VulkanImage(VulkanWindow* vulkanWindow, uint32_t width, uint32_t height, VkBufferUsageFlags usage, uint32_t memoryTypeIndex);
    ~VulkanImage();

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;
    VulkanImage& operator=(VulkanImage&& other) noexcept;

    VkImage getImage() const { return m_image; }
    VkImageView getImageView() const { return m_imageView; }
    VkSampler getSampler() const { return m_sampler; }

    void destroy() { cleanup(); }

private:
    void createImage();
    void allocateMemory();
    void createImageView();
    void createSampler();
    void cleanup();
    void swap(VulkanImage& other) noexcept;

    VulkanWindow* m_vulkanWindow = nullptr;
    uint32_t m_width{};
    uint32_t m_height{};
    VkImageUsageFlags m_usage{};
    uint32_t m_memoryTypeIndex{};

    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_memory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    
    VkResult m_result = VK_NOT_READY;
    QVulkanDeviceFunctions* m_deviceFunctions = nullptr;
};
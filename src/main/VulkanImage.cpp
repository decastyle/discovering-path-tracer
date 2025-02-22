#include "VulkanImage.h"

VulkanImage::VulkanImage(QVulkanWindow* vulkanWindow, uint32_t width, uint32_t height, VkBufferUsageFlags usage, uint32_t memoryTypeIndex)
    : m_vulkanWindow(vulkanWindow), 
      m_width(width), 
      m_height(height), 
      m_usage(usage), 
      m_memoryTypeIndex(memoryTypeIndex)
{
    m_device = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_device);

    createImage();
    allocateMemory();
    createImageView();
    createSampler();
}

VulkanImage::~VulkanImage()
{
    cleanup();
}

void VulkanImage::createImage()
{
    VkImageCreateInfo imageCreateInfo
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R32G32B32A32_SFLOAT,  // 4-component float format
        .extent        = { m_width, m_height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = m_usage,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    m_result = m_deviceFunctions->vkCreateImage(m_device, &imageCreateInfo, nullptr, &m_image);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create image (error code: %d)", m_result);
        return;
    }
}

void VulkanImage::allocateMemory()
{
    VkMemoryRequirements memoryRequirements;
    m_deviceFunctions->vkGetImageMemoryRequirements(m_device, m_image, &memoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = m_memoryTypeIndex
    };

    m_result = m_deviceFunctions->vkAllocateMemory(m_device, &memoryAllocateInfo, nullptr, &m_memory);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to allocate image memory (error code: %d)", m_result);
        return;
    }

    m_deviceFunctions->vkBindImageMemory(m_device, m_image, m_memory, 0);
}

void VulkanImage::createImageView()
{
    VkImageViewCreateInfo imageViewCreateInfo
    {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = m_image,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R32G32B32A32_SFLOAT,
        .components       = {}, // Identity mapping (R->R, G->G, etc.)
        .subresourceRange = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };    

    m_result = m_deviceFunctions->vkCreateImageView(m_device, &imageViewCreateInfo, nullptr, &m_imageView);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create image view (error code: %d)", m_result);
        return;
    }
}

void VulkanImage::createSampler()
{
    VkSamplerCreateInfo samplerCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE, // TODO: anisotropy support
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };

    m_result = m_deviceFunctions->vkCreateSampler(m_device, &samplerCreateInfo, nullptr, &m_sampler);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create sampler (error code: %d)", m_result);
        return;
    }
}

void VulkanImage::cleanup()
{
    if (m_sampler)
    {
        m_deviceFunctions->vkDestroySampler(m_device, m_sampler, nullptr);
    }
    if (m_imageView) 
    {
        m_deviceFunctions->vkDestroyImageView(m_device, m_imageView, nullptr);
    }
    if (m_memory) 
    {
        m_deviceFunctions->vkFreeMemory(m_device, m_memory, nullptr);
    }
    if (m_image) 
    {
        m_deviceFunctions->vkDestroyImage(m_device, m_image, nullptr);
    }
}
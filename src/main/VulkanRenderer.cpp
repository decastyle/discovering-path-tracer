#include "VulkanRenderer.h"
#include "VulkanWindow.h"
#include <QVulkanFunctions>
#include <QFile>
#include <QObject>
#include "Camera.h"
#include <QThread>

static const uint64_t render_width     = 1024; // TODO: Pass this data dynamically through Qt's GUI
static const uint64_t render_height    = 1024;
static const uint32_t workgroup_width  = 16;
static const uint32_t workgroup_height = 16;

// TODO: Include .obj loader and a button "Load .obj" to load vertexData (update vertex input locations)

// static float vertexData[] = {
//     // Position (x, y, z)  // Normal (nx, ny, nz) // UV (u, v)
//
//     // Front face (Z = 1.0f)
//     -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
//      0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,
//      0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,
//
//      0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,
//     -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
//
//     // Back face (Z = -1.0f)
//     -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,
//      0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,
//      0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,
//
//      0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,
//     -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,
//
//     // Left face (X = -1.0f)
//     -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//     -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
//     -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//
//     -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//     -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//
//     // Right face (X = 1.0f)
//      0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//      0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//      0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
//
//      0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//      0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//      0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
//
//     // Top face (Y = 1.0f)
//     -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,
//      0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
//      0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,
//
//      0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
//     -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,
//     -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,
//
//     // Bottom face (Y = -1.0f)
//     -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
//      0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,
//      0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
//
//      0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
//     -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
// };

static float vertexData[] = {
    // Front face (Z = 1.0f)
    -0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
     0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,
     0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,

     0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,
    -0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
    -0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
};

static const int UNIFORM_MATRIX_DATA_SIZE = 16 * sizeof(float);
static const int UNIFORM_VECTOR_DATA_SIZE = 3 * sizeof(float);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VulkanRenderer::VulkanRenderer(VulkanWindow *vulkanWindow)
    : m_vulkanWindow(vulkanWindow)
{   
    const QList<int> counts = vulkanWindow->supportedSampleCounts();

    qDebug() << "Supported sample counts:" << counts;
    for (int s = 16; s >= 4; s /= 2) {
        if (counts.contains(s)) {
            qDebug("Requesting sample count %d", s);
            m_vulkanWindow->setSampleCount(s);
            break;
        }
    }
}

void VulkanRenderer::initResources()
{
    VkResult result{};
    // QVulkanInstance *inst = m_vulkanWindow->vulkanInstance();
    VkDevice device = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(device);
    m_vulkanWindow->getVulkanRayTracer()->deviceReady();

    uint32_t graphicsQueueFamilyIndex = m_vulkanWindow->findQueueFamilyIndex(m_vulkanWindow->physicalDevice(), VK_QUEUE_GRAPHICS_BIT);
    if (graphicsQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable graphics queue family found!");

    m_deviceFunctions->vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &m_graphicsQueue);

    const int concurrentFrameCount = m_vulkanWindow->concurrentFrameCount(); 
    qDebug() << "Concurrent frame count:" << concurrentFrameCount;
    
    const VkPhysicalDeviceLimits *pdevLimits = &m_vulkanWindow->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    const float maxSamplerAnisotropy = pdevLimits->maxSamplerAnisotropy;
    qDebug("Uniform buffer offset alignment is %u", (uint) uniAlign);

    /////////////////////////////////////////////////////////////////////
    // Create buffers
    /////////////////////////////////////////////////////////////////////

    // Create vertex buffer
    VkBufferCreateInfo vertexBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(vertexData),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_deviceFunctions->vkCreateBuffer(device, &vertexBufferCreateInfo, nullptr, &m_vertexBuffer);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create vertex buffer (error code: %d)", result);
        return;
    }

    VkMemoryRequirements vertexBufferMemoryRequirements;
    m_deviceFunctions->vkGetBufferMemoryRequirements(device, m_vertexBuffer, &vertexBufferMemoryRequirements);

    VkMemoryAllocateInfo vertexBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = vertexBufferMemoryRequirements.size,
        .memoryTypeIndex = m_vulkanWindow->hostVisibleMemoryIndex()
    };

    VkResult result = m_deviceFunctions->vkAllocateMemory(device, &vertexBufferMemoryAllocateInfo, nullptr, &m_vertexMemory);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to allocate vertex memory (error code: %d)", result);
        return; 
    }

    result = m_deviceFunctions->vkBindBufferMemory(device, m_vertexBuffer, m_vertexMemory, 0);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to bind vertex buffer memory (error code: %d)", result);
        return; 
    }

    // Create staging buffer
    VkBufferCreateInfo vertexStagingBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(vertexData),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    VkResult result = m_deviceFunctions->vkCreateBuffer(device, &vertexStagingBufferCreateInfo, nullptr, &m_vertexStagingBuffer);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create staging buffer (error code: %d)", result);
        return;
    }

    VkMemoryRequirements vertexStagingBufferMemoryRequirements;
    m_deviceFunctions->vkGetBufferMemoryRequirements(device, m_vertexStagingBuffer, &vertexStagingBufferMemoryRequirements);

    VkMemoryAllocateInfo vertexStagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = vertexStagingBufferMemoryRequirements.size,
        .memoryTypeIndex = m_vulkanWindow->hostVisibleMemoryIndex()
    };

    result = m_deviceFunctions->vkAllocateMemory(device, &vertexStagingBufferMemoryAllocateInfo, nullptr, &m_vertexStagingMemory);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to allocate staging memory (error code: %d)", result);
        return;
    }

    result = m_deviceFunctions->vkBindBufferMemory(device, m_vertexStagingBuffer, m_vertexStagingMemory, 0);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to bind staging buffer memory (error code: %d)", result);
        return;
    }

    quint8 *pStaging = nullptr;
    result = m_deviceFunctions->vkMapMemory(device, m_vertexStagingMemory, 0, vertexStagingBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pStaging));
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to map staging memory (error code: %d)", result);
        return;
    }

    memcpy(pStaging, vertexData, sizeof(vertexData));
    m_deviceFunctions->vkUnmapMemory(device, m_vertexStagingMemory);

    // Create uniform buffer
    const VkDeviceSize uniformBufferDeviceSize = aligned(UNIFORM_MATRIX_DATA_SIZE, uniAlign) + aligned(UNIFORM_VECTOR_DATA_SIZE, uniAlign);

    VkBufferCreateInfo uniformBufferCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = uniformBufferDeviceSize * concurrentFrameCount,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };
    
    result = m_deviceFunctions->vkCreateBuffer(device, &uniformBufferCreateInfo, nullptr, &m_uniformBuffer);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create uniform buffer (error code: %d)", result);
        return;
    }

    VkMemoryRequirements uniformBufferMemoryRequirements;
    m_deviceFunctions->vkGetBufferMemoryRequirements(device, m_uniformBuffer, &uniformBufferMemoryRequirements);

    VkMemoryAllocateInfo uniformBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = uniformBufferMemoryRequirements.size,
        .memoryTypeIndex = m_vulkanWindow->hostVisibleMemoryIndex()
    };

    result = m_deviceFunctions->vkAllocateMemory(device, &uniformBufferMemoryAllocateInfo, nullptr, &m_uniformMemory);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to allocate uniform memory (error code: %d)", result);
        return;
    }

    result = m_deviceFunctions->vkBindBufferMemory(device, m_uniformBuffer, m_uniformMemory, 0);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to bind uniform buffer memory (error code: %d)", result);
        return;
    }

    quint8 *pUniform = nullptr;
    result = m_deviceFunctions->vkMapMemory(device, m_uniformMemory, 0, uniformBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pUniform));
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to map uniform memory (error code: %d)", result);
        return;
    }

    QMatrix4x4 identityMatrix;

    for (int i = 0; i < concurrentFrameCount; ++i) 
    {
        const VkDeviceSize offset = i * uniformBufferDeviceSize;
        memcpy(pUniform + offset, identityMatrix.constData(), 16 * sizeof(float));

        m_uniformBufferInfo[i] = VkDescriptorBufferInfo{
            .buffer = m_uniformBuffer,
            .offset = offset,
            .range = uniformBufferDeviceSize
        };
    }
    m_deviceFunctions->vkUnmapMemory(device, m_uniformMemory);

    /////////////////////////////////////////////////////////////////////
    // Create image and image view
    /////////////////////////////////////////////////////////////////////

    VkImageCreateInfo imageCreateInfo 
    {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R32G32B32A32_SFLOAT,  // 4-component float format
        .extent        = { render_width, render_height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    result = m_deviceFunctions->vkCreateImage(device, &imageCreateInfo, nullptr, &m_renderImage);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create image (error code: %d)", result);
        return;
    }

    VkMemoryRequirements storageImageMemoryRequirements;
    m_deviceFunctions->vkGetImageMemoryRequirements(device, m_renderImage, &storageImageMemoryRequirements);
    
    VkMemoryAllocateInfo memoryAllocateInfo 
    {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = storageImageMemoryRequirements.size,
        .memoryTypeIndex = m_vulkanWindow->deviceLocalMemoryIndex()
    };    
    
    result = m_deviceFunctions->vkAllocateMemory(device, &memoryAllocateInfo, nullptr, &m_renderImageMemory);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to allocate image memory (error code: %d)", result);
        return;
    }
    
    m_deviceFunctions->vkBindImageMemory(device, m_renderImage, m_renderImageMemory, 0);

    VkImageViewCreateInfo imageViewCreateInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = m_renderImage,
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

    result = m_deviceFunctions->vkCreateImageView(device, &imageViewCreateInfo, nullptr, &m_renderImageView);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create image view (error code: %d)", result);
        return;
    }

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
            .anisotropyEnable = VK_FALSE, // TODO: Check if anisotropy is supported
            .maxAnisotropy = maxSamplerAnisotropy,
            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,
            .minLod = 0.0f,
            .maxLod = VK_LOD_CLAMP_NONE,
            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

    result = m_deviceFunctions->vkCreateSampler(device, &samplerCreateInfo, nullptr, &m_textureSampler);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create sampler (error code: %d)", result);
        return;
    }

    /////////////////////////////////////////////////////////////////////
    // Create command buffer
    /////////////////////////////////////////////////////////////////////

    VkCommandPoolCreateInfo commandPoolCreateInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };
    
    VkCommandPool commandPool;

    result = m_deviceFunctions->vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create command pool (error code: %d)", result);
        return;
    }

    VkCommandBufferAllocateInfo commandBufferAllocateInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer commandBuffer;
    result = m_deviceFunctions->vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to allocate command buffer (error code: %d)", result);
        return;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    
    result = m_deviceFunctions->vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to begin command buffer (error code: %d)", result);
        return;
    }

    VkImageMemoryBarrier imageMemoryBarrierToTransferDst
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_renderImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };  

    m_deviceFunctions->vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr, 
    1, &imageMemoryBarrierToTransferDst);   

    VkImageCopy bufferCopyRegion
    {
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .srcOffset = {0, 0, 0},
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .dstOffset = {0, 0, 0},
        .extent = {
            .width = render_width,
            .height = render_height,
            .depth = 1
        }
    };

    VulkanRayTracer *vulkanRayTracer = m_vulkanWindow->getVulkanRayTracer();
    VkImage storageImage = vulkanRayTracer->getStorageImage(); 

    m_deviceFunctions->vkCmdCopyImage(
        commandBuffer, 
        storageImage, 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        m_renderImage, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, &bufferCopyRegion);

    VkImageMemoryBarrier imageMemoryBarrierToShaderRead
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_renderImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };  

    m_deviceFunctions->vkCmdPipelineBarrier(commandBuffer,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr, 
    1, &imageMemoryBarrierToShaderRead);   

    result = m_deviceFunctions->vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to end command buffer (error code: %d)", result);
        return;
    }

    VkSubmitInfo submitInfo 
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = nullptr,
        .pWaitDstStageMask    = nullptr,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = nullptr
    };   

    result = m_deviceFunctions->vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to submit command buffer to graphics queue (error code: %d)", result);
        return;
    }

    result = m_deviceFunctions->vkQueueWaitIdle(m_graphicsQueue);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to wait for graphics queue (error code: %d)", result);
        return;
    }

    /////////////////////////////////////////////////////////////////////
    // Pipeline shader stages
    /* More info...
    It uses .spv aliases defined in shaders.qrc
    All shaders are compiled automatically during CMake configuration with glslangValidator.exe
    */
    /////////////////////////////////////////////////////////////////////

    VkShaderModule vertexShaderModule = m_vulkanWindow->createShaderModule(QStringLiteral(":/color_vert.spv"));
    VkShaderModule fragmentShaderModule = m_vulkanWindow->createShaderModule(QStringLiteral(":/color_frag.spv"));

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShaderModule,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShaderModule,
            .pName = "main",
            .pSpecializationInfo = nullptr
        }
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline vertex input state
    /* More info...
    The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be passed to the vertex shader. It describes this in roughly two ways:

    Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
    Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    */
    /////////////////////////////////////////////////////////////////////

    // TODO: Update vertex input binding description, it should include vertices, normals and UV coordinates

    VkVertexInputBindingDescription vertexInputBindingDescription = {
        .binding = 0,
        .stride = 8 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertexInputAttributeDescription[] = {
        { // position
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        },
        { // normal
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 3 * sizeof(float)
        },
        { // uv
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 6 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = vertexInputAttributeDescription
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline input assembly state
    /* More info...
    The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the vertices and if primitive restart should be enabled. The former is specified in the topology member and can have values like:

        VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
    Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you can specify the indices to use yourself. This allows you to perform optimizations like reusing vertices. If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline viewport state
    /* More info...
    
    Viewport(s) and scissor rectangle(s) can either be specified as a static part of the pipeline or as a dynamic state set in the command buffer. While the former is more in line with the other states it's often convenient to make viewport and scissor state dynamic as it gives you a lot more flexibility. This is very common and all implementations can handle this dynamic state without a performance penalty.

    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,  // Optional: set to nullptr if not using custom viewports
        .scissorCount = 1,
        .pScissors = nullptr    // Optional: set to nullptr if not using custom scissors
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline rasterization state
    /* More info...
    The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo structure.
    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp = 0.0f,         // Optional
        .depthBiasSlopeFactor = 0.0f,    // Optional
        .lineWidth = 1.0f
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline multisample state
    /* More info...
    The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the ways to perform anti-aliasing. It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel. This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur. Because it doesn't need to run the fragment shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than simply rendering to a higher resolution and then downscaling. Enabling it requires enabling a GPU feature.
    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = m_vulkanWindow->sampleCountFlagBits(),
        .sampleShadingEnable = VK_TRUE,
        .minSampleShading = 1.0f,           // Optional
        .pSampleMask = nullptr,            // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE      // Optional
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline depth stencil state
    /* More info...

    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,  // Optional: set to VK_FALSE if not using depth bounds test
        .stencilTestEnable = VK_FALSE,      // Optional: set to VK_FALSE if not using stencil test
        .front = {},                        // Optional: stencil front settings if needed
        .back = {},                         // Optional: stencil back settings if needed
        .minDepthBounds = 0.0f,             // Optional
        .maxDepthBounds = 1.0f              // Optional
    };


    /////////////////////////////////////////////////////////////////////
    // Pipeline color blend state
    /////////////////////////////////////////////////////////////////////

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &pipelineColorBlendAttachmentState,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // Optional
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline dynamic state
    /* More info...
    While most of the pipeline state needs to be baked into the pipeline state, a limited amount of the state can actually be changed without recreating the pipeline at draw time. Examples are the size of the viewport, line width and blend constants. 

    This will cause the configuration of these values to be ignored and you will be able (and required) to specify the data at drawing time. This results in a more flexible setup and is very common for things like viewport and scissor state, which would result in a more complex setup when being baked into the pipeline state.
    */
    /////////////////////////////////////////////////////////////////////

    VkDynamicState dynamicStates[] = { 
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR 
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
        .pDynamicStates = dynamicStates
    };
    
    /////////////////////////////////////////////////////////////////////
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descriptorPoolSizes[]
    {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = uint32_t(concurrentFrameCount)   
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = uint32_t(concurrentFrameCount)
        }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = static_cast<uint32_t>(concurrentFrameCount),
        .poolSizeCount = 2,
        .pPoolSizes = descriptorPoolSizes
    };

    result = m_deviceFunctions->vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor pool: %d", result);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[] 
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 2,
        .pBindings = descriptorSetLayoutBinding
    };

    result = m_deviceFunctions->vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create descriptor set layout (error code: %d)", result);
        return;
    }

    for (int i = 0; i < concurrentFrameCount; ++i) {

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };

        result = m_deviceFunctions->vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &m_descriptorSet[i]);
        if (result != VK_SUCCESS)
        {
            qWarning("Failed to allocate descriptor set (error code: %d)", result);
            return;
        }

        VkWriteDescriptorSet uniformBufferWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = m_descriptorSet[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &m_uniformBufferInfo[i],
            .pTexelBufferView = nullptr
        };

        VkDescriptorImageInfo m_descriptorImageInfo 
        {
            .sampler = m_textureSampler,
            .imageView = m_renderImageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet storageImageWrite
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = m_descriptorSet[i],
            .dstBinding = 1, // Binding 1 is for storage image
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &m_descriptorImageInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };

        VkWriteDescriptorSet descriptorWrites[] = { storageImageWrite, uniformBufferWrite };
        
        m_deviceFunctions->vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
    }

    /////////////////////////////////////////////////////////////////////
    // Pipeline layout
    /////////////////////////////////////////////////////////////////////

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = nullptr
    };

    result = m_deviceFunctions->vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create pipeline cache (error code: %d)", result);
        return;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    result = m_deviceFunctions->vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create pipeline layout (error code: %d)", result);
        return;
    }

    /////////////////////////////////////////////////////////////////////
    // Graphics pipeline
    /////////////////////////////////////////////////////////////////////

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &pipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
        .pTessellationState = nullptr,
        .pViewportState = &pipelineViewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = &pipelineMultisampleStateCreateInfo,
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &pipelineColorBlendStateCreateInfo,
        .pDynamicState = &pipelineDynamicStateCreateInfo,
        .layout = m_pipelineLayout,
        .renderPass = m_vulkanWindow->defaultRenderPass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = m_deviceFunctions->vkCreateGraphicsPipelines(device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to create graphics pipeline (error code: %d)", result);
        return;
    }

    /////////////////////////////////////////////////////////////////////
    // Clean up
    /////////////////////////////////////////////////////////////////////

    if (vertexShaderModule)
        m_deviceFunctions->vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    if (fragmentShaderModule)
        m_deviceFunctions->vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
}

void VulkanRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");

    m_vulkanWindow->getCamera()->swapChainUpdate();
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");

    VkDevice device = m_vulkanWindow->device();

    if (m_pipeline) {
        m_deviceFunctions->vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout) {
        m_deviceFunctions->vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineCache) {
        m_deviceFunctions->vkDestroyPipelineCache(device, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout) {
        m_deviceFunctions->vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool) {
        m_deviceFunctions->vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_vertexBuffer) {
        m_deviceFunctions->vkDestroyBuffer(device, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexStagingBuffer) {
        m_deviceFunctions->vkDestroyBuffer(device, m_vertexStagingBuffer, nullptr);
        m_vertexStagingBuffer = VK_NULL_HANDLE;
    }

    if (m_uniformBuffer) {
        m_deviceFunctions->vkDestroyBuffer(device, m_uniformBuffer, nullptr);
        m_uniformBuffer = VK_NULL_HANDLE;
    }

    if (m_vertexStagingMemory) {
        m_deviceFunctions->vkFreeMemory(device, m_vertexStagingMemory, nullptr);
        m_vertexStagingMemory = VK_NULL_HANDLE;
    }

    if (m_uniformMemory) {
        m_deviceFunctions->vkFreeMemory(device, m_uniformMemory, nullptr);
        m_uniformMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::startNextFrame()
{
    m_renderTimer.start();

    VkDevice device = m_vulkanWindow->device();
    QSize swapChainSize = m_vulkanWindow->swapChainImageSize();
    VkCommandBuffer commandBuffer = m_vulkanWindow->currentCommandBuffer();
    uint32_t currentFrame = m_vulkanWindow->currentFrame();

    /////////////////////////////////////////////////////////////////////
    // Copy staging buffer to vertex buffer
    /////////////////////////////////////////////////////////////////////

    VkBufferCopy bufferCopyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sizeof(vertexData)
    };

    m_deviceFunctions->vkCmdCopyBuffer(commandBuffer, m_vertexStagingBuffer, m_vertexBuffer, 1, &bufferCopyRegion);

    /////////////////////////////////////////////////////////////////////
    // Pipeline barrier to ensure staging buffer copy completes before rendering
    /////////////////////////////////////////////////////////////////////

    // TODO: Do not copy if the data is the same

    VkMemoryBarrier memoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    };

    m_deviceFunctions->vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,         
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,    
        0,
        1, &memoryBarrier,  // Global memory barrier
        0, nullptr,
        0, nullptr
    );

    /////////////////////////////////////////////////////////////////////
    // Begin render pass
    /////////////////////////////////////////////////////////////////////

    VkClearColorValue clearColorValue = { .float32 = { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkClearDepthStencilValue clearDepthStencilValue = { .depth = 1.0f, .stencil = 0 };

    VkClearValue clearValues[3] = {
        { .color = clearColorValue },
        { .depthStencil = clearDepthStencilValue },
        { .color = clearColorValue }
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = m_vulkanWindow->defaultRenderPass(),
        .framebuffer = m_vulkanWindow->currentFramebuffer(),
        .renderArea = { .offset = { 0, 0 }, .extent = { static_cast<uint32_t>(swapChainSize.width()), static_cast<uint32_t>(swapChainSize.height()) } },
        .clearValueCount = static_cast<uint32_t>(m_vulkanWindow->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2),
        .pClearValues = clearValues
    };

    m_deviceFunctions->vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    /////////////////////////////////////////////////////////////////////
    // Map projection matrix
    /////////////////////////////////////////////////////////////////////
    
    quint8 *p;

    VkResult result = m_deviceFunctions->vkMapMemory(device, m_uniformMemory, m_uniformBufferInfo[currentFrame].offset,
            UNIFORM_MATRIX_DATA_SIZE + UNIFORM_VECTOR_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if (result != VK_SUCCESS)
    {
        qWarning("Failed to map memory");
        return;
    }

    Camera *camera = m_vulkanWindow->getCamera();

    QMatrix4x4 projectionMatrix = camera->getProjectionMatrix();
    QVector3D cameraPosition = camera->getPosition();

    memcpy(p, projectionMatrix.constData(), 16 * sizeof(float));
    memcpy(p + UNIFORM_MATRIX_DATA_SIZE, &cameraPosition, 3 * sizeof(float));

    m_deviceFunctions->vkUnmapMemory(device, m_uniformMemory);

    /////////////////////////////////////////////////////////////////////
    // Bind pipeline
    /////////////////////////////////////////////////////////////////////

    m_deviceFunctions->vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    m_deviceFunctions->vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                               &m_descriptorSet[m_vulkanWindow->currentFrame()], 0, nullptr);

    VkDeviceSize vertexBufferOffset = 0;
    m_deviceFunctions->vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_vertexBuffer, &vertexBufferOffset);

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(swapChainSize.width()),
        .height = static_cast<float>(swapChainSize.height()),
        .minDepth = 0,
        .maxDepth = 1
    };

    m_deviceFunctions->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = { 
            static_cast<uint32_t>(viewport.width), 
            static_cast<uint32_t>(viewport.height) 
        }
    };
    m_deviceFunctions->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Calculate vertex count (each vertex has 8 elements: position, normal, and UV)
    // (x, y, z, x, y, z, u, v)
    const uint32_t vertexCount = sizeof(vertexData) / sizeof(vertexData[0]) / 8;  

    m_deviceFunctions->vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

    m_deviceFunctions->vkCmdEndRenderPass(commandBuffer);

    qint64 renderTimeNs = m_renderTimer.nsecsElapsed();
    double fps = 1e9/(static_cast<double>(renderTimeNs));

    qDebug().nospace() << "Render time: " << (renderTimeNs / 1.0e6) << " ms, FPS: " << fps;

    m_vulkanWindow->frameReady();
    m_vulkanWindow->requestUpdate(); 
}
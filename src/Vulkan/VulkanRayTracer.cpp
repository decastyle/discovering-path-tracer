#include "VulkanRayTracer.h"
#define TINYOBJLOADER_IMPLEMENTATION

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "BoundingVolumeHierarchy.h"

static const uint64_t render_width     = 1024;
static const uint64_t render_height    = 1024;
static const uint32_t workgroup_width  = 16;
static const uint32_t workgroup_height = 16;

// TODO: because VkDevice is only valid between VulkanRenderer::initResources() and VulkanRenderer::releaseResources(), RayTracer may fault while doing work on separate thread.
// Possible solution is to create separate device, but then VkCmdCopyImage is no longer possible between two separate VkDevice
void VulkanRayTracer::deviceReady()
{
    initComputePipeline();
}

void VulkanRayTracer::initComputePipeline()
{
    VkResult result{};
    VkDevice dev = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(dev);

    uint32_t computeQueueFamilyIndex = m_vulkanWindow->findQueueFamilyIndex(m_vulkanWindow->physicalDevice(), VK_QUEUE_COMPUTE_BIT);
    if (computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");

    m_deviceFunctions->vkGetDeviceQueue(dev, computeQueueFamilyIndex, 0, &m_computeQueue);
    VulkanCommandPool computeCommandPool = VulkanCommandPool(m_vulkanWindow, computeQueueFamilyIndex);

    /////////////////////////////////////////////////////////////////////
    // Load the mesh of the first shape from an OBJ file
    /////////////////////////////////////////////////////////////////////

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile("../scenes/Sylveon.obj")) { // TODO: Better scene loading
        qDebug() << "Failed to load .obj: " << reader.Error();
    }

    const std::vector<tinyobj::real_t>& objVertices = reader.GetAttrib().GetVertices(); // [x0, y0, z0, x1, y1, z1, ...]
    const std::vector<tinyobj::shape_t>& objShapes = reader.GetShapes(); // All shapes in the file

    // Store indices from all shapes
    std::vector<uint32_t> objIndices; // [v0, v1, v2, v3, v4, v5, ...]

    for (const tinyobj::shape_t& objShape : objShapes) {
        for (const tinyobj::index_t& index : objShape.mesh.indices) {
            objIndices.push_back(index.vertex_index);
        }
    }

    BVH bvh(objVertices, objIndices);
    qDebug() << "BVH built with" << bvh.getNodes().size() << "nodes";

    VkDeviceSize vertexSize = objVertices.size() * sizeof(tinyobj::real_t);
    m_vertexBuffer = VulkanBuffer(m_vulkanWindow, 
                                    vertexSize,
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    m_vulkanWindow->deviceLocalMemoryIndex());
        
    m_vertexStagingBuffer = VulkanBuffer(m_vulkanWindow, 
                                        vertexSize,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        m_vulkanWindow->hostVisibleMemoryIndex());

    m_vertexStagingBuffer.copyData(objVertices.data(), vertexSize); 

    {
        VulkanCommandBuffer commandBuffer = VulkanCommandBuffer(m_vulkanWindow, computeCommandPool.getCommandPool(), m_computeQueue);

        commandBuffer.beginSingleTimeCommandBuffer();

            VkBufferCopy bufferCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexSize
        };

        m_deviceFunctions->vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), 
                                            m_vertexStagingBuffer.getBuffer(), 
                                            m_vertexBuffer.getBuffer(), 
                                            1, &bufferCopyRegion);

        VkMemoryBarrier memoryBarrier = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
        };

        m_deviceFunctions->vkCmdPipelineBarrier(
            commandBuffer.getCommandBuffer(),
            VK_PIPELINE_STAGE_TRANSFER_BIT,         
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,    
            0,
            1, &memoryBarrier,  // Global memory barrier
            0, nullptr,
            0, nullptr
        );
        
        commandBuffer.endSubmitAndWait();
    }


    // std::vector<Triangle> triangles;

    // for (const tinyobj::shape_t& objShape : objShapes) 
    // {
    //     for (size_t i = 0; i < objShape.mesh.indices.size(); i += 3) 
    //     {
    //         Triangle triangle;
    //         triangle.v0 = glm::vec3(
    //             objVertices[objShape.mesh.indices[i].vertex_index * 3],
    //             objVertices[objShape.mesh.indices[i].vertex_index * 3 + 1],
    //             objVertices[objShape.mesh.indices[i].vertex_index * 3 + 2]
    //         );
    //         triangle.v1 = glm::vec3(
    //             objVertices[objShape.mesh.indices[i + 1].vertex_index * 3],
    //             objVertices[objShape.mesh.indices[i + 1].vertex_index * 3 + 1],
    //             objVertices[objShape.mesh.indices[i + 1].vertex_index * 3 + 2]
    //         );
    //         triangle.v2 = glm::vec3(
    //             objVertices[objShape.mesh.indices[i + 2].vertex_index * 3],
    //             objVertices[objShape.mesh.indices[i + 2].vertex_index * 3 + 1],
    //             objVertices[objShape.mesh.indices[i + 2].vertex_index * 3 + 2]
    //         );

    //         triangles.push_back(triangle);
    //     }
    // }

    /////////////////////////////////////////////////////////////////////
    // Create image and image view
    /////////////////////////////////////////////////////////////////////

    VkImageCreateInfo imageInfo 
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
        .usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    result = m_deviceFunctions->vkCreateImage(dev, &imageInfo, nullptr, &m_storageImage);
    if (result != VK_SUCCESS)
        qDebug("Failed to create image: %d", result);

    VkMemoryRequirements storageImageMemoryRequirements;
    m_deviceFunctions->vkGetImageMemoryRequirements(dev, m_storageImage, &storageImageMemoryRequirements);
    
    VkMemoryAllocateInfo allocInfo 
    {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = storageImageMemoryRequirements.size,
        .memoryTypeIndex = m_vulkanWindow->deviceLocalMemoryIndex()
    };    
    
    result = m_deviceFunctions->vkAllocateMemory(dev, &allocInfo, nullptr, &m_storageImageMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate image memory: %d", result);
    
    m_deviceFunctions->vkBindImageMemory(dev, m_storageImage, m_storageImageMemory, 0);

    VkImageViewCreateInfo viewInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .image            = m_storageImage,
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

    result = m_deviceFunctions->vkCreateImageView(dev, &viewInfo, nullptr, &m_storageImageView);
    if (result != VK_SUCCESS)
        qDebug("Failed to create image view: %d", result);

    /////////////////////////////////////////////////////////////////////
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descPoolSizes[]
    {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1  
        }
    };

    VkDescriptorPoolCreateInfo descPoolInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = descPoolSizes
    };

    result = m_deviceFunctions->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor pool: %d", result);

    VkDescriptorSetLayoutBinding layoutBinding[] 
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
    };

    VkDescriptorSetLayoutCreateInfo descLayoutInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = layoutBinding
    };

    result = m_deviceFunctions->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor set layout: %d", result);

    VkDescriptorSetAllocateInfo descSetAllocInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_descriptorSetLayout
    };

    result = m_deviceFunctions->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &m_descriptorSet);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate descriptor set: %d", result);
    
    VkDescriptorImageInfo descImageInfo 
    {
        .sampler = VK_NULL_HANDLE,
        .imageView = m_storageImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet storageImageWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = 0, // Binding 0 is for storage image
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &descImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet descriptorWrites[] = { storageImageWrite };
    
    m_deviceFunctions->vkUpdateDescriptorSets(dev, 1, descriptorWrites, 0, nullptr);

    /////////////////////////////////////////////////////////////////////
    // Create command buffer
    /////////////////////////////////////////////////////////////////////

    VkCommandPoolCreateInfo cmdPoolInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = computeQueueFamilyIndex
    };
    
    VkCommandPool cmdPool;

    result = m_deviceFunctions->vkCreateCommandPool(dev, &cmdPoolInfo, nullptr, &cmdPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create command pool: %d", result);

    VkShaderModule rayTraceModule = m_vulkanWindow->createShaderModule(QStringLiteral(":/raytrace_comp.spv"));

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo 
    {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,                
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .module              = rayTraceModule,
        .pName               = "main",
        .pSpecializationInfo = nullptr            
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &m_descriptorSetLayout,  
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr
    };
    
    result = m_deviceFunctions->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
        qDebug("Failed to create pipeline layout: %d", result);

    VkComputePipelineCreateInfo pipelineCreateInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = shaderStageCreateInfo,
        .layout             = m_pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = -1
    };

    result = m_deviceFunctions->vkCreateComputePipelines(dev, m_pipelineCache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_computePipeline);
    if (result != VK_SUCCESS)
        qDebug("Failed to create compute pipeline: %d", result);

    VkCommandBufferAllocateInfo cmdAllocInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = cmdPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmdBuffer;
    result = m_deviceFunctions->vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate command buffer: %d", result);

    VkCommandBufferBeginInfo beginInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    
    result = m_deviceFunctions->vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    if (result != VK_SUCCESS)
        qDebug("Failed to begin command buffer: %d", result);

    VkImageMemoryBarrier imageMemoryBarrierToGeneral
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_storageImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };  

    m_deviceFunctions->vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr, 
    1, &imageMemoryBarrierToGeneral);   

    m_deviceFunctions->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

    m_deviceFunctions->vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

    m_deviceFunctions->vkCmdDispatch(cmdBuffer, (uint32_t(render_width) + workgroup_width - 1) / workgroup_width,
                (uint32_t(render_height) + workgroup_height - 1) / workgroup_height, 1);

    VkImageMemoryBarrier imageMemoryBarrierToTransferSrc
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_storageImage,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };  

    m_deviceFunctions->vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr, 
    1, &imageMemoryBarrierToTransferSrc);   

    result = m_deviceFunctions->vkEndCommandBuffer(cmdBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to end command buffer: %d", result);

    VkSubmitInfo submitInfo 
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = nullptr,
        .pWaitDstStageMask    = nullptr,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &cmdBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = nullptr
    };    

    result = m_deviceFunctions->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
        qDebug("Failed to submit command buffer to compute queue: %d", result);

    result = m_deviceFunctions->vkQueueWaitIdle(m_computeQueue);
    if (result != VK_SUCCESS)
        qDebug("Failed to wait for compute queue: %d", result);
}

VulkanRayTracer::VulkanRayTracer(VulkanWindow *w)
    : m_vulkanWindow(w)
{
    
}

VkImage VulkanRayTracer::getStorageImage()
{
    return m_storageImage;
}
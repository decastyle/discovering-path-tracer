#include "VulkanRayTracer.h"
#define TINYOBJLOADER_IMPLEMENTATION

#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanCommandPool.h"
#include "BoundingVolumeHierarchy.h"

#include "VulkanWindow.h"

struct PushConstants
{
    uint32_t sample_batch;
};

PushConstants pushConstants;

static const uint64_t render_width     = 1024;
static const uint64_t render_height    = 1024;
static const uint32_t workgroup_width  = 16;
static const uint32_t workgroup_height = 16;

VulkanRayTracer::VulkanRayTracer(VulkanWindow *w)
    : m_vulkanWindow(w){}

void VulkanRayTracer::initRayTracer()
{
    initComputePipeline(); 
}

void VulkanRayTracer::initComputePipeline()
{
    m_device = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_device);

    m_graphicsQueueFamilyIndex = m_vulkanWindow->findQueueFamilyIndex(m_vulkanWindow->physicalDevice(), VK_QUEUE_GRAPHICS_BIT);
    if (m_graphicsQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable graphics queue family found!");

    m_computeQueueFamilyIndex = m_vulkanWindow->findQueueFamilyIndex(m_vulkanWindow->physicalDevice(), VK_QUEUE_COMPUTE_BIT);
    if (m_computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");

    m_deviceFunctions->vkGetDeviceQueue(m_device, m_computeQueueFamilyIndex, 0, &m_computeQueue);
    m_computeCommandPool = VulkanCommandPool(m_vulkanWindow, m_computeQueueFamilyIndex);

    /////////////////////////////////////////////////////////////////////
    // Load the mesh from an OBJ file
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
    // std::vector<tinyobj::real_t> objVertices = { -1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f };
    // std::vector<uint32_t> objIndices = { 0, 1, 2 };

    BVH bvh(objVertices, objIndices);
    // bvh.printBVH(bvh);

    /////////////////////////////////////////////////////////////////////
    // Buffer setup
    /////////////////////////////////////////////////////////////////////

    // Setup vertex buffer
    VkDeviceSize vertexSize = bvh.getVertices().size() * sizeof(tinyobj::real_t);
    m_vertexBuffer          = VulkanBuffer(m_vulkanWindow, 
                                            vertexSize,
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            m_vulkanWindow->deviceLocalMemoryIndex());
        
    m_vertexStagingBuffer   = VulkanBuffer(m_vulkanWindow, 
                                            vertexSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            m_vulkanWindow->hostVisibleMemoryIndex());

    m_vertexStagingBuffer.copyData(bvh.getVertices().data(), vertexSize); 

    // Setup index buffer
    VkDeviceSize indexSize  = bvh.getIndices().size() * sizeof(uint32_t);
    m_indexBuffer           = VulkanBuffer(m_vulkanWindow, 
                                            indexSize,
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            m_vulkanWindow->deviceLocalMemoryIndex());
        
    m_indexStagingBuffer    = VulkanBuffer(m_vulkanWindow, 
                                            indexSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            m_vulkanWindow->hostVisibleMemoryIndex());

    m_indexStagingBuffer.copyData(bvh.getIndices().data(), indexSize); 

    // Setup BVH buffer
    VkDeviceSize BVHSize    = bvh.getNodes().size() * sizeof(BVHNode);
    m_BVHBuffer             = VulkanBuffer(m_vulkanWindow, 
                                            BVHSize,
                                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            m_vulkanWindow->deviceLocalMemoryIndex());
        
    m_BVHStagingBuffer      = VulkanBuffer(m_vulkanWindow, 
                                            BVHSize,
                                            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                            m_vulkanWindow->hostVisibleMemoryIndex());

    m_BVHStagingBuffer.copyData(bvh.getNodes().data(), BVHSize); 

    /////////////////////////////////////////////////////////////////////
    // Copy staging buffers to device local memory buffers
    /////////////////////////////////////////////////////////////////////

    {
        VulkanCommandBuffer commandBuffer = VulkanCommandBuffer(m_vulkanWindow, m_computeCommandPool.getCommandPool(), m_computeQueue);

        commandBuffer.beginSingleTimeCommandBuffer();

        // Copy vertex staging buffer to vertex buffer

            VkBufferCopy vertexBufferCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = vertexSize
        };

        m_deviceFunctions->vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), 
                                            m_vertexStagingBuffer.getBuffer(), 
                                            m_vertexBuffer.getBuffer(), 
                                            1, &vertexBufferCopyRegion);

        // Copy index staging buffer to index buffer

            VkBufferCopy indexBufferCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = indexSize
        };

        m_deviceFunctions->vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), 
                                            m_indexStagingBuffer.getBuffer(), 
                                            m_indexBuffer.getBuffer(), 
                                            1, &indexBufferCopyRegion);

        // Copy BVH staging buffer to BVH buffer

            VkBufferCopy BVHBufferCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = BVHSize
        };

        m_deviceFunctions->vkCmdCopyBuffer(commandBuffer.getCommandBuffer(), 
                                            m_BVHStagingBuffer.getBuffer(), 
                                            m_BVHBuffer.getBuffer(), 
                                            1, &BVHBufferCopyRegion);

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

    /////////////////////////////////////////////////////////////////////
    // Create image
    /////////////////////////////////////////////////////////////////////

    m_storageImage = VulkanImage(m_vulkanWindow, 
                                    render_width, render_height, 
                                    VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                    m_vulkanWindow->deviceLocalMemoryIndex());

    /////////////////////////////////////////////////////////////////////
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descriptorPoolSizes[]
    {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1  // For the storage image
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 3  // For vertex, index, and BVH buffers
        }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,              // Still one descriptor set
        .poolSizeCount = 2,        // Two types: image and buffer
        .pPoolSizes = descriptorPoolSizes
    };

    m_result = m_deviceFunctions->vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create descriptor pool (error code: %d)", m_result);
        return;
    }

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[] = {
        {   // Binding 0: Storage Image
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {   // Binding 1: Vertex Buffer (SSBO)
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {   // Binding 2: Index Buffer (SSBO)
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {   // Binding 3: BVH Buffer (SSBO)
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        }
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 4,
        .pBindings = descriptorSetLayoutBinding
    };

    m_result = m_deviceFunctions->vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create descriptor set layout (error code: %d)", m_result);
        return;
    }

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo 
    {   
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_descriptorSetLayout
    };

    m_result = m_deviceFunctions->vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSet);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to allocate descriptor set (error code: %d)", m_result);
        return;
    }
    
    // Storage Image Info
    VkDescriptorImageInfo descriptorImageInfo = {
        .sampler = VK_NULL_HANDLE,
        .imageView = m_storageImage.getImageView(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    // SSBO Info for Vertex, Index, and BVH Buffers
    VkDescriptorBufferInfo vertexBufferInfo = {
        .buffer = m_vertexBuffer.getBuffer(),
        .offset = 0,
        .range = vertexSize
    };

    VkDescriptorBufferInfo indexBufferInfo = {
        .buffer = m_indexBuffer.getBuffer(),
        .offset = 0,
        .range = indexSize
    };

    VkDescriptorBufferInfo BVHBufferInfo = {
        .buffer = m_BVHBuffer.getBuffer(),
        .offset = 0,
        .range = BVHSize
    };

    VkWriteDescriptorSet storageImageWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &descriptorImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet vertexBufferWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &vertexBufferInfo,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet indexBufferWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = 2,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &indexBufferInfo,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet BVHBufferWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descriptorSet,
        .dstBinding = 3,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &BVHBufferInfo,
        .pTexelBufferView = nullptr
    };


    VkWriteDescriptorSet descriptorWrites[] = { storageImageWrite , vertexBufferWrite , indexBufferWrite , BVHBufferWrite };
    
    m_deviceFunctions->vkUpdateDescriptorSets(m_device, 4, descriptorWrites, 0, nullptr);

    /////////////////////////////////////////////////////////////////////
    // Compute pipeline setup
    /////////////////////////////////////////////////////////////////////

    VkShaderModule computeShaderModule = m_vulkanWindow->createShaderModule(QStringLiteral(":/raytrace_comp.spv"));

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo 
    {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,                
        .stage               = VK_SHADER_STAGE_COMPUTE_BIT,
        .module              = computeShaderModule,
        .pName               = "main",
        .pSpecializationInfo = nullptr            
    };

    VkPushConstantRange pushConstantRange
    {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset     = 0,                              
        .size       = sizeof(PushConstants)
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &m_descriptorSetLayout,  
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange
    };
    
    m_result = m_deviceFunctions->vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    if (m_result != VK_SUCCESS)
        qDebug("Failed to create pipeline layout: %d", m_result);

    VkComputePipelineCreateInfo computePipelineCreateInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = pipelineShaderStageCreateInfo,
        .layout             = m_pipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = -1
    };

    m_result = m_deviceFunctions->vkCreateComputePipelines(m_device, m_pipelineCache, 1, &computePipelineCreateInfo, VK_NULL_HANDLE, &m_computePipeline);
    if (m_result != VK_SUCCESS)
        qDebug("Failed to create compute pipeline: %d", m_result);

    {
        VulkanCommandBuffer commandBuffer = VulkanCommandBuffer(m_vulkanWindow, m_computeCommandPool.getCommandPool(), m_computeQueue);

        commandBuffer.beginSingleTimeCommandBuffer();

        VkImageMemoryBarrier imageMemoryBarrierToTransferSrc
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_storageImage.getImage(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };  

        m_deviceFunctions->vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr, 
        1, &imageMemoryBarrierToTransferSrc); 

        commandBuffer.endSubmitAndWait();
    }

    /////////////////////////////////////////////////////////////////////
    // Main loop
    /////////////////////////////////////////////////////////////////////

    mainLoop();
}

void VulkanRayTracer::mainLoop()
{
    const uint32_t NUM_SAMPLE_BATCHES = 1024;
    for(uint32_t sampleBatch = 0; sampleBatch < NUM_SAMPLE_BATCHES; sampleBatch++)
    {
        VulkanCommandBuffer commandBuffer = VulkanCommandBuffer(m_vulkanWindow, m_computeCommandPool.getCommandPool(), m_computeQueue);

        commandBuffer.beginSingleTimeCommandBuffer();

        VkImageMemoryBarrier imageMemoryBarrierToGeneral
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_GENERAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_storageImage.getImage(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };  

        m_deviceFunctions->vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(),
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr, 
        1, &imageMemoryBarrierToGeneral);   

        m_deviceFunctions->vkCmdBindPipeline(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

        m_deviceFunctions->vkCmdBindDescriptorSets(commandBuffer.getCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descriptorSet, 0, nullptr);

        // Push push constants:
        pushConstants.sample_batch = sampleBatch;
        vkCmdPushConstants(commandBuffer.getCommandBuffer(),
                        m_pipelineLayout,
                        VK_SHADER_STAGE_COMPUTE_BIT,
                        0,
                        sizeof(PushConstants),
                        &pushConstants);               

        m_deviceFunctions->vkCmdDispatch(commandBuffer.getCommandBuffer(),
                    (uint32_t(render_width) + workgroup_width - 1) / workgroup_width,
                    (uint32_t(render_height) + workgroup_height - 1) / workgroup_height, 1);

        VkImageMemoryBarrier imageMemoryBarrierToTransferSrc
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_storageImage.getImage(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };  

        m_deviceFunctions->vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(),
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr, 
        1, &imageMemoryBarrierToTransferSrc);   

        commandBuffer.endSubmitAndWait();

        VkFence fence = commandBuffer.getFence();

        m_vulkanWindow->getVulkanRenderer()->copyStorageImage(fence);

        qDebug("Storage image copied! sampleBatch: %i", sampleBatch);
    }
}
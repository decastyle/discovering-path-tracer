#include <vulkan/vulkan.h>
#include <vector>
#include "vulkanraytracer.h"

static const uint64_t render_width     = 800;
static const uint64_t render_height    = 600;
static const uint32_t workgroup_width  = 16;
static const uint32_t workgroup_height = 8;

uint32_t VulkanRayTracer::findComputeQueueFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
            return i;  
        }
    }
    
    return UINT32_MAX; 
}

VkShaderModule VulkanRayTracer::createShaderModule(const QString& filename)
{
    VkShaderModule shaderModule;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(filename));
        shaderModule = VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = static_cast<size_t>(blob.size()),
        .pCode = reinterpret_cast<const uint32_t *>(blob.constData())
    };


    VkResult result = m_devFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) 
    {
        qWarning("Failed to create shader module: %d", result);
        shaderModule = VK_NULL_HANDLE;
    }
    else
    {
        qDebug("Shaders loaded successfully!");
    }

    return shaderModule;
}

void VulkanRayTracer::onDeviceReady()
{
    static int initialized = 0;

    if(initialized == 0)
    {
        initComputePipeline();

        initialized = 1;
    }
}

void VulkanRayTracer::initComputePipeline()
{
    VkDevice dev = m_window->device();
    qDebug() << dev;
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);
    qDebug() << m_devFuncs;

    VkResult result;

    uint32_t computeQueueFamilyIndex = findComputeQueueFamilyIndex(m_window->physicalDevice());
    if (computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");

    vkGetDeviceQueue(dev, computeQueueFamilyIndex, 0, &m_computeQueue);

    /////////////////////////////////////////////////////////////////////
    // Create storage buffer
    /////////////////////////////////////////////////////////////////////

    VkDeviceSize       bufferSizeBytes = render_width * render_height * 3 * sizeof(float);
    VkBufferCreateInfo vertexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = bufferSizeBytes,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &vertexBufferInfo, nullptr, &m_storageBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create storage buffer: %d", result);

    VkMemoryRequirements storageBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_storageBuffer, &storageBufferMemoryRequirements);

    VkMemoryAllocateInfo storageBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = storageBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &storageBufferMemoryAllocateInfo, nullptr, &m_storageMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate storage memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_storageBuffer, m_storageMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind storage buffer memory: %d", result);

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

    result = m_devFuncs->vkCreateImage(dev, &imageInfo, nullptr, &m_storageImage);
    if (result != VK_SUCCESS)
        qDebug("Failed to create image: %d", result);

    VkMemoryRequirements storageImageMemoryRequirements;
    m_devFuncs->vkGetImageMemoryRequirements(dev, m_storageImage, &storageImageMemoryRequirements);
    
    VkMemoryAllocateInfo allocInfo 
    {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext           = nullptr,
        .allocationSize  = storageImageMemoryRequirements.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };    
    
    result = m_devFuncs->vkAllocateMemory(dev, &allocInfo, nullptr, &m_storageImageMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate image memory: %d", result);
    
    m_devFuncs->vkBindImageMemory(dev, m_storageImage, m_storageImageMemory, 0);

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

    result = m_devFuncs->vkCreateImageView(dev, &viewInfo, nullptr, &m_storageImageView);
    if (result != VK_SUCCESS)
        qDebug("Failed to create image view: %d", result);

    /////////////////////////////////////////////////////////////////////
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descPoolSizes[]
    {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1  
        },
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
        .poolSizeCount = 2,
        .pPoolSizes = descPoolSizes
    };

    result = m_devFuncs->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_descPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor pool: %d", result);

    VkDescriptorSetLayoutBinding layoutBinding[] 
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 1,
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
        .bindingCount = 2,
        .pBindings = layoutBinding
    };

    result = m_devFuncs->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_descSetLayout);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor set layout: %d", result);

    VkDescriptorSetAllocateInfo descSetAllocInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = m_descPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_descSetLayout
    };

    result = m_devFuncs->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &m_descSet);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate descriptor set: %d", result);

    VkDescriptorBufferInfo m_storageBufferInfo
    {
        .buffer = m_storageBuffer,  
        .offset = 0,   
        .range  = VK_WHOLE_SIZE
    };  
    
    VkDescriptorImageInfo descImageInfo 
    {
        .sampler = VK_NULL_HANDLE,
        .imageView = m_storageImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet storageBufferWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descSet,
        .dstBinding = 0, // Binding 0 is for storage buffer
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &m_storageBufferInfo,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet storageImageWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descSet,
        .dstBinding = 1, // Binding 1 is for storage image
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &descImageInfo,
        .pBufferInfo = nullptr,
        .pTexelBufferView = nullptr
    };

    VkWriteDescriptorSet descriptorWrites[] = { storageBufferWrite, storageImageWrite };
    vkUpdateDescriptorSets(dev, 2, descriptorWrites, 0, nullptr);
    
    m_devFuncs->vkUpdateDescriptorSets(dev, 1, descriptorWrites, 0, nullptr);

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

    result = vkCreateCommandPool(dev, &cmdPoolInfo, nullptr, &cmdPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create command pool: %d", result);

    VkShaderModule rayTraceModule = createShaderModule(QStringLiteral(":/raytrace_comp.spv"));

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
        .pSetLayouts            = &m_descSetLayout,  
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr
    };
    
    result = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
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

    result = m_devFuncs->vkCreateComputePipelines(dev, m_pipelineCache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_computePipeline);
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
    result = vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate command buffer: %d", result);

    VkCommandBufferBeginInfo beginInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    
    result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
    if (result != VK_SUCCESS)
        qDebug("Failed to begin command buffer: %d", result);

    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelineLayout, 0, 1, &m_descSet, 0, nullptr);

    vkCmdDispatch(cmdBuffer, (uint32_t(render_width) + workgroup_width - 1) / workgroup_width,
                (uint32_t(render_height) + workgroup_height - 1) / workgroup_height, 1);

    VkMemoryBarrier memoryBarrier
    {
        .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext         = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,  
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT    
    };    

    VkImageMemoryBarrier imageMemoryBarrier 
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_HOST_READ_BIT,
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

    vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_HOST_BIT,
    0,
    1, &memoryBarrier,
    0, nullptr, 
    1, &imageMemoryBarrier);   

    result = vkEndCommandBuffer(cmdBuffer);
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

    result = vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    if (result != VK_SUCCESS)
        qDebug("Failed to submit command buffer to compute queue: %d", result);

    result = vkQueueWaitIdle(m_computeQueue);
    if (result != VK_SUCCESS)
        qDebug("Failed to wait for compute queue: %d", result);

    qDebug() << "VulkanRayTracer";

    void* mappedMemory = nullptr;
    vkMapMemory(dev, m_storageMemory, 0, bufferSizeBytes, 0, &mappedMemory); 
    assert(mappedMemory != nullptr);

    qDebug() << "Memory mapped: " << mappedMemory;

    vkUnmapMemory(dev, m_storageMemory);

}

VulkanRayTracer::VulkanRayTracer(VulkanWindow *w)
    : m_window(w)
{
    // TODO: Initialize compute pipeline
}

#include <vulkan/vulkan.h>
#include <vector>
#include "vulkanraytracer.h"

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

    static const uint64_t render_width     = 800;
    static const uint64_t render_height    = 600;
    static const uint32_t workgroup_width  = 16;
    static const uint32_t workgroup_height = 8;

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
        .flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
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
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descPoolSizes = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1  
    };

    VkDescriptorPoolCreateInfo descPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &descPoolSizes
    };

    result = m_devFuncs->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_descPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor pool: %d", result);

    VkDescriptorSetLayoutBinding layoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &layoutBinding
    };

    result = m_devFuncs->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_descSetLayout);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor set layout: %d", result);

    VkDescriptorSetAllocateInfo descSetAllocInfo = {
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
        .range  = bufferSizeBytes
    };     

    VkWriteDescriptorSet descWrite = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = m_descSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = nullptr,
        .pBufferInfo = &m_storageBufferInfo,
        .pTexelBufferView = nullptr
    };
    
    m_devFuncs->vkUpdateDescriptorSets(dev, 1, &descWrite, 0, nullptr);

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

    vkCmdPipelineBarrier(cmdBuffer,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_HOST_BIT,
    0,
    1, &memoryBarrier,
    0, nullptr, 0, nullptr);                               

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

}

VulkanRayTracer::VulkanRayTracer(VulkanWindow *w)
    : m_window(w)
{
    // TODO: Initialize compute pipeline
}

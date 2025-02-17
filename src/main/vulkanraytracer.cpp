#include <vulkan/vulkan.h>
#include <vector>
#include "vulkanraytracer.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "common.h"




PushConstants pushConstants;

static const uint64_t render_width     = 1024;
static const uint64_t render_height    = 1024;
static const uint32_t workgroup_width  = 16;
static const uint32_t workgroup_height = 16;








uint32_t VulkanRayTracer::findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit)
{
    uint32_t queueFamilyCount = 0;
    m_window->vulkanInstance()->functions()->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    m_window->vulkanInstance()->functions()->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & bit) {
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
        initRayTracing();

        initialized = 1;
    }
}









void VulkanRayTracer::initRayTracing()
{
    VkResult result{};

    VkDevice dev = m_window->device();

    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);

    uint32_t computeQueueFamilyIndex = findQueueFamilyIndex(m_window->physicalDevice(), VK_QUEUE_COMPUTE_BIT);
    if (computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");

    qDebug() << "Compute Queue Index:" << computeQueueFamilyIndex;

    m_devFuncs->vkGetDeviceQueue(dev, computeQueueFamilyIndex, 0, &m_computeQueue);















    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(dev, "vkGetAccelerationStructureBuildSizesKHR");

    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(dev, "vkCreateAccelerationStructureKHR");

    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR =
        (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(dev, "vkCmdBuildAccelerationStructuresKHR");

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR =
        (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(dev, "vkGetBufferDeviceAddressKHR");

    PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = 
        (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(dev, "vkGetAccelerationStructureDeviceAddressKHR");

    PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = 
        (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(dev, "vkCreateRayTracingPipelinesKHR");


    PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR =
        (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(dev, "vkGetRayTracingShaderGroupHandlesKHR");

    PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR =
        (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(dev, "vkCmdTraceRaysKHR");





    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtPipelineProperties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
        .pNext = nullptr,
        .shaderGroupHandleSize = 0,
        .maxRayRecursionDepth = 0,
        .maxShaderGroupStride = 0,
        .shaderGroupBaseAlignment = 0,
        .shaderGroupHandleCaptureReplaySize = 0,
        .maxRayDispatchInvocationCount = 0,
        .shaderGroupHandleAlignment = 0,
        .maxRayHitAttributeSize = 0
    };

    VkPhysicalDeviceProperties2 physicalDeviceProperties
    {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &rtPipelineProperties,
        .properties = {}
    };

    vkGetPhysicalDeviceProperties2(m_window->physicalDevice(), &physicalDeviceProperties);
    const VkDeviceSize sbtHeaderSize      = rtPipelineProperties.shaderGroupHandleSize;
    const VkDeviceSize sbtBaseAlignment   = rtPipelineProperties.shaderGroupBaseAlignment;
    // const VkDeviceSize sbtHandleAlignment = rtPipelineProperties.shaderGroupHandleAlignment;
    const VkDeviceSize sbtStride = sbtBaseAlignment *  //
                                 ((sbtHeaderSize + sbtBaseAlignment - 1) / sbtBaseAlignment);









    

    /////////////////////////////////////////////////////////////////////
    // Create command pool
    /////////////////////////////////////////////////////////////////////

    VkCommandPoolCreateInfo cmdPoolInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = 0,
        .queueFamilyIndex = computeQueueFamilyIndex
    };
    
    VkCommandPool cmdPool;

    result = m_devFuncs->vkCreateCommandPool(dev, &cmdPoolInfo, nullptr, &cmdPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create command pool: %d", result);




















        
    /////////////////////////////////////////////////////////////////////
    // Load the mesh of the first shape from an OBJ file
    /////////////////////////////////////////////////////////////////////

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile("../scenes/CornellBox-Original.obj")) { // TODO: Better scene loading
        qDebug() << "Failed to load .obj: " << reader.Error();
    }

    const std::vector<tinyobj::real_t>& objVertices = reader.GetAttrib().GetVertices();
    const std::vector<tinyobj::shape_t>& objShapes = reader.GetShapes(); // All shapes in the file

    std::vector<uint32_t> objIndices; // Store indices from all shapes

    for (const tinyobj::shape_t& objShape : objShapes) {
        for (const tinyobj::index_t& index : objShape.mesh.indices) {
            objIndices.push_back(index.vertex_index);
        }
    }

















    /////////////////////////////////////////////////////////////////////
    // Create buffers
    /////////////////////////////////////////////////////////////////////

    // Create vertex buffer
    VkBufferCreateInfo vertexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = objVertices.size() * sizeof(tinyobj::real_t),
        .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &vertexBufferInfo, nullptr, &m_vertexBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create vertex buffer: %d", result);

    VkMemoryRequirements vertexBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_vertexBuffer, &vertexBufferMemoryRequirements);

    VkMemoryAllocateFlagsInfo vertexBufferAllocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 1 // This is the default for single GPU setups
    };

    VkMemoryAllocateInfo vertexBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &vertexBufferAllocFlagsInfo,
        .allocationSize = vertexBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &vertexBufferMemoryAllocateInfo, nullptr, &m_vertexMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate vertex memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_vertexBuffer, m_vertexMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind vertex buffer memory: %d", result);

    // Create staging buffer
    VkBufferCreateInfo vertexStagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = objVertices.size() * sizeof(tinyobj::real_t),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &vertexStagingBufferInfo, nullptr, &m_vertexStagingBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create vertex staging buffer: %d", result);

    VkMemoryRequirements vertexStagingBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_vertexStagingBuffer, &vertexStagingBufferMemoryRequirements);

    VkMemoryAllocateInfo vertexStagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = vertexStagingBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &vertexStagingBufferMemoryAllocateInfo, nullptr, &m_vertexStagingMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate vertex staging memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_vertexStagingBuffer, m_vertexStagingMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind vertex staging buffer memory: %d", result);

    quint8 *pVertexStaging;

    result = m_devFuncs->vkMapMemory(dev, m_vertexStagingMemory, 0, vertexStagingBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pVertexStaging));
    if (result != VK_SUCCESS)
        qDebug("Failed to map vertex staging memory: %d", result);

    memcpy(pVertexStaging, objVertices.data(), objVertices.size() * sizeof(tinyobj::real_t));
    m_devFuncs->vkUnmapMemory(dev, m_vertexStagingMemory);

    // Create index buffer
    VkBufferCreateInfo indexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = objIndices.size() * sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &indexBufferInfo, nullptr, &m_indexBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create index buffer: %d", result);

    VkMemoryRequirements indexBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_indexBuffer, &indexBufferMemoryRequirements);

    VkMemoryAllocateFlagsInfo indexBufferAllocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 1 // This is the default for single GPU setups
    };

    VkMemoryAllocateInfo indexBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &indexBufferAllocFlagsInfo,
        .allocationSize = indexBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &indexBufferMemoryAllocateInfo, nullptr, &m_indexMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate index memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_indexBuffer, m_indexMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind index buffer memory: %d", result);

    // Create staging buffer
    VkBufferCreateInfo indexStagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = objIndices.size() * sizeof(uint32_t),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &indexStagingBufferInfo, nullptr, &m_indexStagingBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create index staging buffer: %d", result);

    VkMemoryRequirements indexStagingBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_indexStagingBuffer, &indexStagingBufferMemoryRequirements);

    VkMemoryAllocateInfo indexStagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = indexStagingBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &indexStagingBufferMemoryAllocateInfo, nullptr, &m_indexStagingMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate index staging memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_indexStagingBuffer, m_indexStagingMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind index staging buffer memory: %d", result);

    quint8 *pIndexStaging;

    result = m_devFuncs->vkMapMemory(dev, m_indexStagingMemory, 0, indexStagingBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pIndexStaging));
    if (result != VK_SUCCESS)
        qDebug("Failed to map index staging memory: %d", result);

    memcpy(pIndexStaging, objIndices.data(), objIndices.size() * sizeof(uint32_t));
    m_devFuncs->vkUnmapMemory(dev, m_indexStagingMemory);















    /////////////////////////////////////////////////////////////////////
    // Create command buffer (copy vertex and index data)
    /////////////////////////////////////////////////////////////////////

    {
        VkCommandBufferAllocateInfo cmdAllocInfo 
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer cmdBuffer;
        result = m_devFuncs->vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
        if (result != VK_SUCCESS)
            qDebug("Failed to allocate command buffer: %d", result);

        VkCommandBufferBeginInfo beginInfo 
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        
        result = m_devFuncs->vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        if (result != VK_SUCCESS)
            qDebug("Failed to begin command buffer: %d", result);

        /////////////////////////////////////////////////////////////////////
        // Copy vertex staging buffer to vertex buffer
        /////////////////////////////////////////////////////////////////////

        VkBufferCopy vertexCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = objVertices.size() * sizeof(tinyobj::real_t)
        };

        m_devFuncs->vkCmdCopyBuffer(cmdBuffer, m_vertexStagingBuffer, m_vertexBuffer, 1, &vertexCopyRegion);

        /////////////////////////////////////////////////////////////////////
        // Copy index staging buffer to index buffer
        /////////////////////////////////////////////////////////////////////

        VkBufferCopy indexCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = objIndices.size() * sizeof(uint32_t),
        };

        m_devFuncs->vkCmdCopyBuffer(cmdBuffer, m_indexStagingBuffer, m_indexBuffer, 1, &indexCopyRegion);

        /////////////////////////////////////////////////////////////////////
        // Pipeline barrier to ensure staging buffer copy completes before rendering
        /////////////////////////////////////////////////////////////////////

        VkBufferMemoryBarrier indexBufferBarrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_INDEX_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_indexBuffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };

        VkBufferMemoryBarrier vertexBufferBarrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer = m_vertexBuffer,
            .offset = 0,
            .size = VK_WHOLE_SIZE
        };

        VkBufferMemoryBarrier bufferBarriers[] = { indexBufferBarrier, vertexBufferBarrier };

        m_devFuncs->vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,         
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,    
            0,
            0, nullptr,
            2, bufferBarriers,
            0, nullptr
        );

        result = m_devFuncs->vkEndCommandBuffer(cmdBuffer);
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

        result = m_devFuncs->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
            qDebug("Failed to submit command buffer to compute queue: %d", result);

        result = m_devFuncs->vkQueueWaitIdle(m_computeQueue);
        if (result != VK_SUCCESS)
            qDebug("Failed to wait for compute queue: %d", result);
    }

    qDebug() << "VULKANRAYTRACER.CPP after copying vertex and index data";

























    /////////////////////////////////////////////////////////////////////
    // Setup Bottom Level Acceleration Structure
    /////////////////////////////////////////////////////////////////////

    uint32_t primitiveCount = static_cast<uint32_t>(objIndices.size() / 3);

    VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
        .pNext = nullptr,
        .buffer = m_vertexBuffer
    };
    
    VkDeviceAddress vertexBufferAddress = vkGetBufferDeviceAddressKHR(dev, &vertexBufferDeviceAddressInfo);

    VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
        .pNext = nullptr,
        .buffer = m_indexBuffer
    };

    VkDeviceAddress indexBufferAddress = vkGetBufferDeviceAddressKHR(dev, &indexBufferDeviceAddressInfo);

    VkAccelerationStructureGeometryTrianglesDataKHR triangles
    {
        .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .pNext         = nullptr,
        .vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT,
        .vertexData    = {.deviceAddress = vertexBufferAddress},
        .vertexStride  = 3 * sizeof(float),
        .maxVertex     = static_cast<uint32_t>(objVertices.size() / 3 - 1),
        .indexType     = VK_INDEX_TYPE_UINT32,
        .indexData     = {.deviceAddress = indexBufferAddress},
        .transformData = {.deviceAddress = 0}  // No transform
    };

    // Create a VkAccelerationStructureGeometryKHR object that says it handles opaque triangles and points to the above:
    VkAccelerationStructureGeometryKHR geometry
    {
        .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext        = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry     = {.triangles = triangles},
        .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildRangeInfoKHR offsetInfo
    {
        .primitiveCount  = primitiveCount,  // Number of triangles
        .primitiveOffset = 0,  // Offset added when looking up triangles
        .firstVertex     = 0,  // Offset added when looking up vertices in the vertex buffer
        .transformOffset = 0   // Offset added when looking up transformation matrices, if we used them
    };

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo
    {
        .sType                      = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext                      = nullptr,
        .type                       = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags                      = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode                       = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure   = VK_NULL_HANDLE, 
        .dstAccelerationStructure   = VK_NULL_HANDLE,
        .geometryCount              = 1,
        .pGeometries                = &geometry,
        .ppGeometries               = nullptr, 
        .scratchData                = {}
    };

    VkAccelerationStructureBuildSizesInfoKHR sizeInfo
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        .pNext = nullptr,
        .accelerationStructureSize = 0,
        .updateScratchSize = 0,
        .buildScratchSize = 0
    };

    vkGetAccelerationStructureBuildSizesKHR(
        dev,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &primitiveCount,
        &sizeInfo
    );








    // Create BLAS buffer
    VkBuffer blasBuffer;

    VkBufferCreateInfo blasBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeInfo.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &blasBufferInfo, nullptr, &blasBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create BLAS buffer: %d", result);

    VkMemoryRequirements blasBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, blasBuffer, &blasBufferMemoryRequirements);

    VkMemoryAllocateFlagsInfo blasBufferAllocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 1 // This is the default for single GPU setups
    };

    VkMemoryAllocateInfo blasBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &blasBufferAllocFlagsInfo,
        .allocationSize = blasBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    VkDeviceMemory blasMemory;

    result = m_devFuncs->vkAllocateMemory(dev, &blasBufferMemoryAllocateInfo, nullptr, &blasMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate BLAS memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, blasBuffer, blasMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind BLAS buffer memory: %d", result);



    VkAccelerationStructureCreateInfoKHR blasCreateInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .createFlags = 0,
        .buffer = blasBuffer,
        .offset = 0,
        .size = sizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .deviceAddress = 0
    };

    VkAccelerationStructureKHR blas;
    vkCreateAccelerationStructureKHR(dev, &blasCreateInfo, nullptr, &blas);










    // Create scratch buffer
    VkBuffer blasScratchBuffer;

    VkBufferCreateInfo blasScratchBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeInfo.buildScratchSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &blasScratchBufferInfo, nullptr, &blasScratchBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create scratch buffer: %d", result);

    VkMemoryRequirements blasScratchBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, blasScratchBuffer, &blasScratchBufferMemoryRequirements);

    VkMemoryAllocateFlagsInfo blasScratchBufferAllocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 1 // This is the default for single GPU setups
    };

    VkMemoryAllocateInfo blasScratchBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &blasScratchBufferAllocFlagsInfo,
        .allocationSize = blasScratchBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    VkDeviceMemory scratchMemory;

    result = m_devFuncs->vkAllocateMemory(dev, &blasScratchBufferMemoryAllocateInfo, nullptr, &scratchMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate scratch memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, blasScratchBuffer, scratchMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind scratch buffer memory: %d", result);


    VkBufferDeviceAddressInfo blasScratchBufferAddressInfo{
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext  = nullptr,
        .buffer = blasScratchBuffer
    };

    VkDeviceAddress blasScratchBufferAddress = vkGetBufferDeviceAddressKHR(dev, &blasScratchBufferAddressInfo);

    buildInfo.dstAccelerationStructure = blas;
    buildInfo.scratchData.deviceAddress = blasScratchBufferAddress;

    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &offsetInfo;


























    /////////////////////////////////////////////////////////////////////
    // Create command buffer (build BLAS)
    /////////////////////////////////////////////////////////////////////

    {
        VkCommandBufferAllocateInfo cmdAllocInfo 
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer cmdBuffer;
        result = m_devFuncs->vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
        if (result != VK_SUCCESS)
            qDebug("Failed to allocate command buffer: %d", result);

        VkCommandBufferBeginInfo beginInfo 
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        
        result = m_devFuncs->vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        if (result != VK_SUCCESS)
            qDebug("Failed to begin command buffer: %d", result);

        vkCmdBuildAccelerationStructuresKHR(
            cmdBuffer,
            1,
            &buildInfo,
            &pRangeInfo
        );

        VkMemoryBarrier barrier
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
        };

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            0,
            1, &barrier,
            0, nullptr,
            0, nullptr
        );

        result = m_devFuncs->vkEndCommandBuffer(cmdBuffer);
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

        result = m_devFuncs->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
            qDebug("Failed to submit command buffer to compute queue: %d", result);

        result = m_devFuncs->vkQueueWaitIdle(m_computeQueue);
        if (result != VK_SUCCESS)
            qDebug("Failed to wait for compute queue: %d", result);
    }























    /////////////////////////////////////////////////////////////////////
    // Setup Top Level Acceleration Structure
    /////////////////////////////////////////////////////////////////////

    VkAccelerationStructureDeviceAddressInfoKHR blasAddressInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .pNext = nullptr,
        .accelerationStructure = blas
    };

    VkDeviceAddress blasDeviceAddress = vkGetAccelerationStructureDeviceAddressKHR(dev, &blasAddressInfo);

    std::vector<VkAccelerationStructureInstanceKHR> instances
    {
        VkAccelerationStructureInstanceKHR
        {
            .transform{
                .matrix{
                    { 1.0f, 0.0f, 0.0f, 0.0f },
                    { 0.0f, 1.0f, 0.0f, 0.0f },
                    { 0.0f, 0.0f, 1.0f, 0.0f }
                }
            },
            .instanceCustomIndex = 0,
            .mask = 0xFF,
            .instanceShaderBindingTableRecordOffset = 0,
            .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            .accelerationStructureReference = blasDeviceAddress
        }
    };

    VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

    VkBuffer instanceBuffer;
    VkDeviceMemory instanceMemory;

    VkBufferCreateInfo instanceBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr, 
        .flags = 0,      
        .size = instanceBufferSize,
        .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,  
        .pQueueFamilyIndices = nullptr 
    };


    result = m_devFuncs->vkCreateBuffer(dev, &instanceBufferInfo, nullptr, &instanceBuffer);
    if (result != VK_SUCCESS)
            qDebug("Failed to create instance buffer: %d", result);

    VkMemoryRequirements instanceMemReq;
    vkGetBufferMemoryRequirements(dev, instanceBuffer, &instanceMemReq);

    VkMemoryAllocateFlagsInfo instanceAllocFlags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0
    };


    VkMemoryAllocateInfo instanceAllocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &instanceAllocFlags,
        .allocationSize = instanceMemReq.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &instanceAllocInfo, nullptr, &instanceMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate instance memory buffer: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, instanceBuffer, instanceMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind instance buffer: %d", result);

    // Map and copy instance data to GPU
    void* mappedData;
    result = m_devFuncs->vkMapMemory(dev, instanceMemory, 0, instanceBufferSize, 0, &mappedData);
    if (result != VK_SUCCESS)
        qDebug("Failed to map instance memory: %d", result);

    memcpy(mappedData, instances.data(), instanceBufferSize);
    m_devFuncs->vkUnmapMemory(dev, instanceMemory);

    // Get Device Address for Instance Buffer
    VkBufferDeviceAddressInfo instanceBufferAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,\
        .pNext = nullptr,
        .buffer = instanceBuffer
    };

    VkDeviceAddress instanceBufferAddress = vkGetBufferDeviceAddressKHR(dev, &instanceBufferAddressInfo);






    VkAccelerationStructureGeometryKHR tlasGeometry{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
        .geometry = { .instances = {
            .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
            .pNext = nullptr,
            .arrayOfPointers = VK_FALSE,
            .data = { .deviceAddress = instanceBufferAddress }
        }},
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR
    };


    VkAccelerationStructureBuildGeometryInfoKHR tlasBuildInfo{
        .sType                    = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext                    = nullptr,
        .type                     = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags                    = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .geometryCount            = 1,
        .pGeometries              = &tlasGeometry,
        .ppGeometries             = nullptr, 
        .scratchData              = 0
    };



    uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    VkAccelerationStructureBuildSizesInfoKHR tlasSizeInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
        .pNext = nullptr,
        .accelerationStructureSize = 0,
        .updateScratchSize = 0,
        .buildScratchSize = 0
    };


    vkGetAccelerationStructureBuildSizesKHR(
        dev,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &tlasBuildInfo,
        &instanceCount,
        &tlasSizeInfo
    );











    VkBuffer tlasBuffer;
    VkDeviceMemory tlasMemory;

    VkBufferCreateInfo tlasBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = tlasSizeInfo.accelerationStructureSize,
        .usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };



    result = m_devFuncs->vkCreateBuffer(dev, &tlasBufferInfo, nullptr, &tlasBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create TLAS buffer: %d", result);

    VkMemoryRequirements tlasMemReq;
    vkGetBufferMemoryRequirements(dev, tlasBuffer, &tlasMemReq);

    VkMemoryAllocateFlagsInfo tlasAllocFlags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0
    };


    VkMemoryAllocateInfo tlasAllocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &tlasAllocFlags,
        .allocationSize = tlasMemReq.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &tlasAllocInfo, nullptr, &tlasMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate tlas memory: %d", result);
    result = m_devFuncs->vkBindBufferMemory(dev, tlasBuffer, tlasMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind tlas memory: %d", result);

    VkAccelerationStructureCreateInfoKHR tlasCreateInfo{
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .createFlags = 0,
        .buffer = tlasBuffer,
        .offset = 0,
        .size = tlasSizeInfo.accelerationStructureSize,
        .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .deviceAddress = 0
    };


    VkAccelerationStructureKHR tlas;
    vkCreateAccelerationStructureKHR(dev, &tlasCreateInfo, nullptr, &tlas);









    VkBuffer tlasScratchBuffer;
    VkDeviceMemory tlasScratchMemory;

    VkBufferCreateInfo tlasScratchBufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = tlasSizeInfo.buildScratchSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };


    result = m_devFuncs->vkCreateBuffer(dev, &tlasScratchBufferInfo, nullptr, &tlasScratchBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create tlas scratch buffer: %d", result);

    VkMemoryRequirements tlasScratchMemReq;
    vkGetBufferMemoryRequirements(dev, tlasScratchBuffer, &tlasScratchMemReq);

    VkMemoryAllocateFlagsInfo tlasScratchAllocFlags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0
    };


    VkMemoryAllocateInfo tlasScratchAllocInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &tlasScratchAllocFlags,
        .allocationSize = tlasScratchMemReq.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &tlasScratchAllocInfo, nullptr, &tlasScratchMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate tlas scratch memory: %d", result);
    result = m_devFuncs->vkBindBufferMemory(dev, tlasScratchBuffer, tlasScratchMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind tlas scratch memory: %d", result);

    VkBufferDeviceAddressInfo tlasScratchBufferAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = tlasScratchBuffer
    };


    VkDeviceAddress tlasScratchAddress = vkGetBufferDeviceAddressKHR(dev, &tlasScratchBufferAddressInfo);



    tlasBuildInfo.dstAccelerationStructure = tlas;
    tlasBuildInfo.scratchData.deviceAddress = tlasScratchAddress;   

    VkAccelerationStructureBuildRangeInfoKHR tlasRangeInfo{
        .primitiveCount = instanceCount,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0
    };


    const VkAccelerationStructureBuildRangeInfoKHR* tlasRangeInfos[] = { &tlasRangeInfo };






















    /////////////////////////////////////////////////////////////////////
    // Create command buffer (build TLAS)
    /////////////////////////////////////////////////////////////////////

    {
        VkCommandBufferAllocateInfo cmdAllocInfo 
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer cmdBuffer;
        result = m_devFuncs->vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
        if (result != VK_SUCCESS)
            qDebug("Failed to allocate command buffer: %d", result);

        VkCommandBufferBeginInfo beginInfo 
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        
        result = m_devFuncs->vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        if (result != VK_SUCCESS)
            qDebug("Failed to begin command buffer: %d", result);
        
        vkCmdBuildAccelerationStructuresKHR(
            cmdBuffer, 
            1, 
            &tlasBuildInfo, 
            tlasRangeInfos
        );

        VkMemoryBarrier barrier
        {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
            .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR
        };

        vkCmdPipelineBarrier(
            cmdBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            0,
            1, &barrier,
            0, nullptr,
            0, nullptr
        );

        result = m_devFuncs->vkEndCommandBuffer(cmdBuffer);
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

        result = m_devFuncs->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
            qDebug("Failed to submit command buffer to compute queue: %d", result);

        result = m_devFuncs->vkQueueWaitIdle(m_computeQueue);
        if (result != VK_SUCCESS)
            qDebug("Failed to wait for compute queue: %d", result);
    }




























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
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1  
        },
        {
            .type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1  
        },
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1  
        },
    };

    VkDescriptorPoolCreateInfo descPoolInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(std::size(descPoolSizes)),
        .pPoolSizes = descPoolSizes
    };

    result = m_devFuncs->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_descPool);
    if (result != VK_SUCCESS)
        qDebug("Failed to create descriptor pool: %d", result);




    // Here's the list of bindings for the descriptor set layout, from raytrace.comp.glsl:
    // 0 - a storage image (the image `image`)
    // 1 - an acceleration structure (the TLAS)
    // 2 - a storage buffer (the vertex buffer)
    // 3 - a storage buffer (the index buffer)
    VkDescriptorSetLayoutBinding layoutBinding[] 
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 3,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
            .pImmutableSamplers = nullptr
        },
    };

    VkDescriptorSetLayoutCreateInfo descLayoutInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = static_cast<uint32_t>(std::size(layoutBinding)),
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




    
    VkDescriptorImageInfo descImageInfo 
    {
        .sampler = VK_NULL_HANDLE,
        .imageView = m_storageImageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkWriteDescriptorSet storageImageWrite
    {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = nullptr,
        .dstSet           = m_descSet,
        .dstBinding       = 0, // Binding 0 is for storage image
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo       = &descImageInfo,
        .pBufferInfo      = nullptr,
        .pTexelBufferView = nullptr
    };


    VkWriteDescriptorSetAccelerationStructureKHR ASWrite
    {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
        .pNext = nullptr,
        .accelerationStructureCount = 1,
        .pAccelerationStructures    = &tlas
    };

    VkWriteDescriptorSet accelerationWrite
    {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = &ASWrite,
        .dstSet           = m_descSet,
        .dstBinding       = 1, // Binding 1 is for acceleration structure
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
        .pImageInfo       = nullptr,
        .pBufferInfo      = nullptr,
        .pTexelBufferView = nullptr
    };

    VkDescriptorBufferInfo vertexDescriptorBufferInfo
    {
        .buffer = m_vertexBuffer, 
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet vertexWrite
    {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = nullptr,
        .dstSet           = m_descSet,
        .dstBinding       = 2, // Binding 2 is for vertex data
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo       = nullptr,
        .pBufferInfo      = &vertexDescriptorBufferInfo,
        .pTexelBufferView = nullptr
    };


    VkDescriptorBufferInfo indexDescriptorBufferInfo
    {
        .buffer = m_indexBuffer, 
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet indexWrite
    {
        .sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext            = nullptr,
        .dstSet           = m_descSet,
        .dstBinding       = 3, // Binding 3 is for index data
        .dstArrayElement  = 0,
        .descriptorCount  = 1,
        .descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo       = nullptr,
        .pBufferInfo      = &indexDescriptorBufferInfo,
        .pTexelBufferView = nullptr
    };



    VkWriteDescriptorSet descriptorWrites[] = { 
        storageImageWrite, // 0 binding
        accelerationWrite, // 1 binding
        vertexWrite,       // 2 binding
        indexWrite         // 3 binding
    };
    
    m_devFuncs->vkUpdateDescriptorSets(dev, static_cast<uint32_t>(std::size(descriptorWrites)), descriptorWrites, 0, nullptr);
























    /////////////////////////////////////////////////////////////////////
    // Create shader binding table (SBT) and ray tracing pipeline
    /////////////////////////////////////////////////////////////////////

    // VkShaderModule rayGenModule = createShaderModule(QStringLiteral(":/raytrace_rgen.spv"));

    const size_t                                      NUM_C_HIT_SHADERS = 1;
    std::array<VkShaderModule, 2 + NUM_C_HIT_SHADERS> modules;
    modules[0] = createShaderModule(QStringLiteral(":/raytrace_rgen.spv"));
    modules[1] = createShaderModule(QStringLiteral(":/raytrace_rmiss.spv"));
    modules[2] = createShaderModule(QStringLiteral(":/material0_rchit.spv"));

    std::array<VkPipelineShaderStageCreateInfo, 2 + NUM_C_HIT_SHADERS> stages;

    stages[0] = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,                
        .stage               = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
        .module              = modules[0],
        .pName               = "main",
        .pSpecializationInfo = nullptr            
    };
    stages[1] = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,                
        .stage               = VK_SHADER_STAGE_MISS_BIT_KHR,
        .module              = modules[1],
        .pName               = "main",
        .pSpecializationInfo = nullptr            
    };
    stages[2] = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,                
        .stage               = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
        .module              = modules[2],
        .pName               = "main",
        .pSpecializationInfo = nullptr            
    };







    std::array<VkRayTracingShaderGroupCreateInfoKHR, 2 + NUM_C_HIT_SHADERS> groups;


    groups[0] = {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .pNext              = nullptr,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader      = 0,     
        .closestHitShader   = VK_SHADER_UNUSED_KHR,   
        .anyHitShader       = VK_SHADER_UNUSED_KHR,  
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = VK_NULL_HANDLE
    };  

    groups[1] = {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .pNext              = nullptr,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader      = 1,     
        .closestHitShader   = VK_SHADER_UNUSED_KHR,   
        .anyHitShader       = VK_SHADER_UNUSED_KHR,  
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = VK_NULL_HANDLE
    };  
    groups[2] = {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .pNext              = nullptr,
        .type               = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
        .generalShader      = VK_SHADER_UNUSED_KHR,     
        .closestHitShader   = 2,   
        .anyHitShader       = VK_SHADER_UNUSED_KHR,  
        .intersectionShader = VK_SHADER_UNUSED_KHR,
        .pShaderGroupCaptureReplayHandle = VK_NULL_HANDLE
    };  


    

    VkPushConstantRange pushConstantRange
    {
        .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR, 
        .offset     = 0,  
        .size       = sizeof(PushConstants)
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = &m_descSetLayout,  
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange
    };
    
    result = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_raytracingPipelineLayout);
    if (result != VK_SUCCESS)
        qDebug("Failed to create pipeline layout: %d", result);

    VkRayTracingPipelineCreateInfoKHR pipelineCreateInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
        .pNext              = nullptr,
        .flags              = 0,
        .stageCount         = static_cast<uint32_t>(stages.size()),
        .pStages            = stages.data(),
        .groupCount         = static_cast<uint32_t>(groups.size()),
        .pGroups            = groups.data(),
        .maxPipelineRayRecursionDepth = 1, 
        .pLibraryInfo       = VK_NULL_HANDLE,
        .pLibraryInterface  = VK_NULL_HANDLE,
        .pDynamicState      = VK_NULL_HANDLE,
        .layout             = m_raytracingPipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = -1
    };
    
    vkCreateRayTracingPipelinesKHR(dev, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_raytracingPipeline);
    if (result != VK_SUCCESS)
        qDebug("Failed to create compute pipeline: %d", result);







    std::vector<uint8_t> cpuShaderHandleStorage(sbtHeaderSize * groups.size());
    vkGetRayTracingShaderGroupHandlesKHR(dev, m_raytracingPipeline, 0, static_cast<uint32_t>(groups.size()), cpuShaderHandleStorage.size(), cpuShaderHandleStorage.data()); 

    const uint32_t sbtSize = static_cast<uint32_t>(sbtStride * groups.size());









    /////////////////////////////////////////////////////////////////////
    // Create SBT buffer
    /////////////////////////////////////////////////////////////////////

    VkBuffer SBTBuffer = VK_NULL_HANDLE;
    VkDeviceMemory SBTMemory = VK_NULL_HANDLE;

    // Create SBT buffer
    VkBufferCreateInfo SBTBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sbtSize,
        .usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &SBTBufferInfo, nullptr, &SBTBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create SBT buffer: %d", result);

    VkMemoryRequirements SBTBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, SBTBuffer, &SBTBufferMemoryRequirements);
    
    VkMemoryAllocateFlagsInfo SBTAllocFlags{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0
    };

    VkMemoryAllocateInfo SBTBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &SBTAllocFlags,
        .allocationSize = SBTBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &SBTBufferMemoryAllocateInfo, nullptr, &SBTMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate SBT memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, SBTBuffer, SBTMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind SBT buffer memory: %d", result);

    

    quint8 *pSBT;

    result = m_devFuncs->vkMapMemory(dev, SBTMemory, 0, SBTBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pSBT));
    if (result != VK_SUCCESS)
        qDebug("Failed to map SBT memory: %d", result);

    for(size_t groupIndex = 0; groupIndex < groups.size(); groupIndex++)
    {
      memcpy(&pSBT[groupIndex * sbtStride], &cpuShaderHandleStorage[groupIndex * sbtHeaderSize], sbtHeaderSize);
    }

    m_devFuncs->vkUnmapMemory(dev, SBTMemory);



    VkStridedDeviceAddressRegionKHR sbtRayGenRegion, sbtMissRegion, sbtHitRegion, sbtCallableRegion;

    VkBufferDeviceAddressInfo SBTBufferAddressInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = SBTBuffer
    };


    VkDeviceAddress sbtStartAddress = vkGetBufferDeviceAddressKHR(dev, &SBTBufferAddressInfo);


    {
        // The ray generation shader region:
        sbtRayGenRegion.deviceAddress = sbtStartAddress;  // Starts here
        sbtRayGenRegion.stride        = sbtStride;        // Uses this stride
        sbtRayGenRegion.size          = sbtStride;        

        sbtMissRegion      = sbtRayGenRegion;  // The miss shader region:
        sbtMissRegion.deviceAddress = sbtStartAddress + sbtStride;  // Starts sbtStride bytes (1 group) in
        sbtMissRegion.size          = sbtStride;         

        sbtHitRegion      = sbtRayGenRegion;  // The hit group region:
        sbtHitRegion.deviceAddress = sbtStartAddress + 2 * sbtStride;  // Starts 2 * sbtStride bytes (2 groups) in
        sbtHitRegion.size          = sbtStride * NUM_C_HIT_SHADERS;              

        sbtCallableRegion      = sbtRayGenRegion;  // The callable shader region:
        sbtCallableRegion.size = 0;                // Is empty
    }




























    const uint32_t NUM_SAMPLE_BATCHES = 32;


    /////////////////////////////////////////////////////////////////////
    // Create command buffer (dispatch compute pipeline and write to image)
    /////////////////////////////////////////////////////////////////////




    {
        VkCommandBufferAllocateInfo cmdAllocInfo 
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer cmdBuffer;
        result = m_devFuncs->vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
        if (result != VK_SUCCESS)
            qDebug("Failed to allocate command buffer: %d", result);

        VkCommandBufferBeginInfo beginInfo 
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        
        result = m_devFuncs->vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        if (result != VK_SUCCESS)
            qDebug("Failed to begin command buffer: %d", result);



        /////////////////////////////////////////////////////////////////////
        // Pipeline barrier to ensure image layout transition completes before rendering
        /////////////////////////////////////////////////////////////////////

        VkImageMemoryBarrier imageMemoryBarrierToGeneral
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
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

        m_devFuncs->vkCmdPipelineBarrier(cmdBuffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        0,
        0, nullptr,
        0, nullptr, 
        1, &imageMemoryBarrierToGeneral);   




        result = m_devFuncs->vkEndCommandBuffer(cmdBuffer);
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

        result = m_devFuncs->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
            qDebug("Failed to submit command buffer to compute queue: %d", result);

        result = m_devFuncs->vkQueueWaitIdle(m_computeQueue);
        if (result != VK_SUCCESS)
            qDebug("Failed to wait for compute queue: %d", result);
    }














    














    for(uint32_t sampleBatch = 0; sampleBatch < NUM_SAMPLE_BATCHES; sampleBatch++)
    {
        VkCommandBufferAllocateInfo cmdAllocInfo 
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .pNext              = nullptr,
            .commandPool        = cmdPool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        VkCommandBuffer cmdBuffer;
        result = m_devFuncs->vkAllocateCommandBuffers(dev, &cmdAllocInfo, &cmdBuffer);
        if (result != VK_SUCCESS)
            qDebug("Failed to allocate command buffer: %d", result);

        VkCommandBufferBeginInfo beginInfo 
        {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext            = nullptr,
            .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr
        };
        
        result = m_devFuncs->vkBeginCommandBuffer(cmdBuffer, &beginInfo);
        if (result != VK_SUCCESS)
            qDebug("Failed to begin command buffer: %d", result);








        m_devFuncs->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_raytracingPipeline);

        m_devFuncs->vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_raytracingPipelineLayout, 0, 1, &m_descSet, 0, nullptr);





        pushConstants.sample_batch = sampleBatch;
        vkCmdPushConstants(cmdBuffer,                               // Command buffer
                       m_raytracingPipelineLayout,           // Pipeline layout
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR,          // Stage flags
                       0,                                       // Offset
                       sizeof(PushConstants),                   // Size in bytes
                       &pushConstants);                         // Data


        vkCmdTraceRaysKHR(cmdBuffer,           // Command buffer
                      &sbtRayGenRegion,    // Region of memory with ray generation groups
                      &sbtMissRegion,      // Region of memory with miss groups
                      &sbtHitRegion,       // Region of memory with hit groups
                      &sbtCallableRegion,  // Region of memory with callable groups
                      render_width,        // Width of dispatch
                      render_height,       // Height of dispatch
                      1);                  // Depth of dispatch



        if(sampleBatch == NUM_SAMPLE_BATCHES - 1)
        {
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
                .image = m_storageImage,
                .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };  

            m_devFuncs->vkCmdPipelineBarrier(cmdBuffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr, 
            1, &imageMemoryBarrierToTransferSrc);   
        }






        result = m_devFuncs->vkEndCommandBuffer(cmdBuffer);
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

        result = m_devFuncs->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        if (result != VK_SUCCESS)
            qDebug("Failed to submit command buffer to compute queue: %d", result);

        result = m_devFuncs->vkQueueWaitIdle(m_computeQueue);
        if (result != VK_SUCCESS)
            qDebug("Failed to wait for compute queue: %d", result);

        qDebug("Rendered sample batch index %d.\n", sampleBatch);
    }

    qDebug() << "VULKANRAYTRACER.CPP after compute write to image";
    
}

VulkanRayTracer::VulkanRayTracer(VulkanWindow *w)
    : m_window(w)
{
    
}

VkImage VulkanRayTracer::getStorageImage()
{
    return m_storageImage;
}
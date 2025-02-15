#include <vulkan/vulkan.h>
#include <vector>
#include "vulkanraytracer.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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
        initComputePipeline();

        initialized = 1;
    }
}

void VulkanRayTracer::initComputePipeline()
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
        .pNext = nullptr,
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
        }
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

    VkDescriptorSetLayoutBinding layoutBinding[] 
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
            .pImmutableSamplers = nullptr
        }
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


    VkWriteDescriptorSet descriptorWrites[] = { storageImageWrite , accelerationWrite };
    
    m_devFuncs->vkUpdateDescriptorSets(dev, static_cast<uint32_t>(std::size(descriptorWrites)), descriptorWrites, 0, nullptr);





















    /////////////////////////////////////////////////////////////////////
    // Create compute pipeline
    /////////////////////////////////////////////////////////////////////

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
    
    result = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_computePipelineLayout);
    if (result != VK_SUCCESS)
        qDebug("Failed to create pipeline layout: %d", result);

    VkComputePipelineCreateInfo pipelineCreateInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext              = nullptr,
        .flags              = 0,
        .stage              = shaderStageCreateInfo,
        .layout             = m_computePipelineLayout,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex  = -1
    };

    result = m_devFuncs->vkCreateComputePipelines(dev, m_computePipelineCache, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &m_computePipeline);
    if (result != VK_SUCCESS)
        qDebug("Failed to create compute pipeline: %d", result);






















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









        m_devFuncs->vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline);

        m_devFuncs->vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, 0, 1, &m_descSet, 0, nullptr);

        m_devFuncs->vkCmdDispatch(cmdBuffer, (uint32_t(render_width) + workgroup_width - 1) / workgroup_width,
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
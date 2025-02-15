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


    VkResult result = m_devFuncs->vkCreateShaderModule(dev, &shaderInfo, nullptr, &shaderModule);
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

    VkResult result;

    VkInstance vkInstance = m_instance->vkInstance();

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(vkInstance, &physicalDeviceCount, physicalDevices.data());

    // Pick the first discrete GPU (or any logic you prefer)
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto& pd : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(pd, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            physicalDevice = pd;
            break;
        }
    }
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("No discrete GPU found");
    }





    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
        .pNext = nullptr,
        .rayTracingPipeline = VK_TRUE,
        .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
        .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
        .rayTracingPipelineTraceRaysIndirect = VK_FALSE,
        .rayTraversalPrimitiveCulling = VK_FALSE
    };


    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructureFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
        .pNext = nullptr,
        .accelerationStructure = VK_TRUE,
        .accelerationStructureCaptureReplay = VK_FALSE,
        .accelerationStructureIndirectBuild = VK_FALSE,
        .accelerationStructureHostCommands = VK_FALSE,
        .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE
    };


    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = nullptr, 
        .bufferDeviceAddress = VK_TRUE,
        .bufferDeviceAddressCaptureReplay = VK_FALSE,
        .bufferDeviceAddressMultiDevice = VK_FALSE
    };


    VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
        .pNext = nullptr,
        .shaderInputAttachmentArrayDynamicIndexing = VK_FALSE,
        .shaderUniformTexelBufferArrayDynamicIndexing = VK_FALSE,
        .shaderStorageTexelBufferArrayDynamicIndexing = VK_FALSE,
        .shaderUniformBufferArrayNonUniformIndexing = VK_FALSE,
        .shaderSampledImageArrayNonUniformIndexing = VK_FALSE,
        .shaderStorageBufferArrayNonUniformIndexing = VK_FALSE,
        .shaderStorageImageArrayNonUniformIndexing = VK_FALSE,
        .shaderInputAttachmentArrayNonUniformIndexing = VK_FALSE,
        .shaderUniformTexelBufferArrayNonUniformIndexing = VK_FALSE,
        .shaderStorageTexelBufferArrayNonUniformIndexing = VK_FALSE,
        .descriptorBindingUniformBufferUpdateAfterBind = VK_FALSE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_FALSE,
        .descriptorBindingStorageImageUpdateAfterBind = VK_FALSE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_FALSE,
        .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE,
        .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE,
        .descriptorBindingUpdateUnusedWhilePending = VK_FALSE,
        .descriptorBindingPartiallyBound = VK_FALSE,
        .descriptorBindingVariableDescriptorCount = VK_FALSE,
        .runtimeDescriptorArray = VK_FALSE
    };

    // Enable what you need
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    accelStructureFeatures.accelerationStructure = VK_TRUE;
    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    descriptorIndexingFeatures.descriptorBindingPartiallyBound = VK_TRUE;
    descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;

    // Link the chain
    rayTracingPipelineFeatures.pNext = &accelStructureFeatures;
    accelStructureFeatures.pNext = &bufferDeviceAddressFeatures;
    bufferDeviceAddressFeatures.pNext = &descriptorIndexingFeatures;

    // Query support
    VkPhysicalDeviceFeatures2 features2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = nullptr, 
        .features = {}   
    };

    features2.pNext = &rayTracingPipelineFeatures;
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);
    

    uint32_t computeQueueFamilyIndex = findQueueFamilyIndex(physicalDevice, VK_QUEUE_COMPUTE_BIT);
    if (computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");


    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = computeQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };


    // Extensions you need
    const char* deviceExtensions[] = {
        // VK_KHR_SWAPCHAIN_EXTENSION_NAME,  // optional, only if needed later
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
    };

    

    VkDeviceCreateInfo deviceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features2, 
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,                
        .ppEnabledLayerNames = nullptr,        
        .enabledExtensionCount = static_cast<uint32_t>(std::size(deviceExtensions)),
        .ppEnabledExtensionNames = deviceExtensions,
        .pEnabledFeatures = nullptr 
    };



    
    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &dev) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ray tracing Vulkan device");
    }

    
    m_devFuncs = m_instance->deviceFunctions(dev);
    qDebug() << m_devFuncs;

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";
    PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(dev, "vkGetAccelerationStructureBuildSizesKHR");

    PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(dev, "vkCreateAccelerationStructureKHR");

    PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR =
        (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(dev, "vkCmdBuildAccelerationStructuresKHR");

    PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR =
        (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(dev, "vkGetBufferDeviceAddressKHR");



    qDebug() << "Compute Queue Index:" << computeQueueFamilyIndex;

    vkGetDeviceQueue(dev, computeQueueFamilyIndex, 0, &m_computeQueue);

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";

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

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";

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

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";

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



    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";



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

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";

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
            .size = sizeof(objVertices)
        };

        m_devFuncs->vkCmdCopyBuffer(cmdBuffer, m_vertexStagingBuffer, m_vertexBuffer, 1, &vertexCopyRegion);

        /////////////////////////////////////////////////////////////////////
        // Copy index staging buffer to index buffer
        /////////////////////////////////////////////////////////////////////

        VkBufferCopy indexCopyRegion = {
            .srcOffset = 0,
            .dstOffset = 0,
            .size = sizeof(objIndices)
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
    // Setup acceleration structures
    /////////////////////////////////////////////////////////////////////

    uint32_t primitiveCount = static_cast<uint32_t>(objIndices.size() / 3);

    VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
        .pNext = nullptr,
        .buffer = m_vertexBuffer
    };

    qDebug() << "AAAAAAAAA";
    
    VkDeviceAddress vertexBufferAddress = vkGetBufferDeviceAddressKHR(dev, &vertexBufferDeviceAddressInfo);

    qDebug() << "AAAAAAAAA";

    VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo
    {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
        .pNext = nullptr,
        .buffer = m_indexBuffer
    };

    qDebug() << "AAAAAAAAA";

    VkDeviceAddress indexBufferAddress = vkGetBufferDeviceAddressKHR(dev, &indexBufferDeviceAddressInfo);

    qDebug() << "AAAAAAAAA";

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

    qDebug() << "AAAAAAAAA";

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
    
    qDebug() << "AAAAAAAAA"; 

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

    qDebug() << "before 1";

    vkGetAccelerationStructureBuildSizesKHR(
        dev,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &primitiveCount,
        &sizeInfo
    );

    qDebug() << "after 1";

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

    qDebug() << "before 2";

    VkAccelerationStructureKHR blas;
    vkCreateAccelerationStructureKHR(dev, &blasCreateInfo, nullptr, &blas);

    qDebug() << "after 2";

    // Create scratch buffer
    VkBuffer scratchBuffer;

    VkBufferCreateInfo scratchBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeInfo.buildScratchSize,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &scratchBufferInfo, nullptr, &scratchBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create scratch buffer: %d", result);

    VkMemoryRequirements scratchBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, scratchBuffer, &scratchBufferMemoryRequirements);

    VkMemoryAllocateFlagsInfo scratchBufferAllocFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = nullptr,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 1 // This is the default for single GPU setups
    };

    VkMemoryAllocateInfo scratchBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &scratchBufferAllocFlagsInfo,
        .allocationSize = scratchBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->deviceLocalMemoryIndex()
    };

    VkDeviceMemory scratchMemory;

    result = m_devFuncs->vkAllocateMemory(dev, &scratchBufferMemoryAllocateInfo, nullptr, &scratchMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate scratch memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, scratchBuffer, scratchMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind scratch buffer memory: %d", result);


    VkBufferDeviceAddressInfo scratchBufferAddressInfo{
        .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = scratchBuffer
    };

    VkDeviceAddress scratchBufferAddress = vkGetBufferDeviceAddressKHR(dev, &scratchBufferAddressInfo);

    buildInfo.dstAccelerationStructure = blas;
    buildInfo.scratchData.deviceAddress = scratchBufferAddress;

    const VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = &offsetInfo;

    /////////////////////////////////////////////////////////////////////
    // Create command buffer (build BLAS)
    /////////////////////////////////////////////////////////////////////

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";

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

    qDebug() << "AAAAAAAAAAAAAAAAAAAAA";

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



    // Create storage staging buffer
    VkBufferCreateInfo storageStagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = storageImageMemoryRequirements.size,
        .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &storageStagingBufferInfo, nullptr, &m_storageStagingBuffer);
    if (result != VK_SUCCESS)
        qDebug("Failed to create storage staging buffer: %d", result);

    VkMemoryRequirements storageStagingBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_storageStagingBuffer, &storageStagingBufferMemoryRequirements);


    VkMemoryAllocateInfo storageStagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = storageStagingBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &storageStagingBufferMemoryAllocateInfo, nullptr, &m_storageStagingMemory);
    if (result != VK_SUCCESS)
        qDebug("Failed to allocate storage staging memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_storageStagingBuffer, m_storageStagingMemory, 0);
    if (result != VK_SUCCESS)
        qDebug("Failed to bind storage staging buffer memory: %d", result);

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
    };

    VkDescriptorSetLayoutCreateInfo descLayoutInfo 
    {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
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

    VkWriteDescriptorSet descriptorWrites[] = { storageImageWrite };
    
    m_devFuncs->vkUpdateDescriptorSets(dev, 1, descriptorWrites, 0, nullptr);

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
        // Copy image to buffer
        /////////////////////////////////////////////////////////////////////



        VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {render_width, render_height, 1}
        };

        m_devFuncs->vkCmdCopyImageToBuffer(cmdBuffer, m_storageImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_storageStagingBuffer, 1, &region);




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

VulkanRayTracer::VulkanRayTracer(QVulkanInstance* instance, VulkanWindow *w)
    : m_instance(instance), m_window(w)
{
    
}

VkImage VulkanRayTracer::getStorageImage()
{
    return m_storageImage;
}
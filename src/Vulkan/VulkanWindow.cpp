#include "VulkanWindow.h"
#include "VulkanRenderer.h"

VulkanWindow::VulkanWindow()
{

    // this->setEnabledFeaturesModifier([this](VkPhysicalDeviceFeatures2 &features2) {
    //     // Create feature structs with the required pNext chain
    //     static VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
    //         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
    //         .pNext = nullptr,
    //         .rayTracingPipeline = VK_TRUE,
    //         .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
    //         .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
    //         .rayTracingPipelineTraceRaysIndirect = VK_FALSE,
    //         .rayTraversalPrimitiveCulling = VK_FALSE
    //     };
    
    //     static VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructureFeatures{
    //         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
    //         .pNext = nullptr,
    //         .accelerationStructure = VK_TRUE,
    //         .accelerationStructureCaptureReplay = VK_FALSE,
    //         .accelerationStructureIndirectBuild = VK_FALSE,
    //         .accelerationStructureHostCommands = VK_FALSE,
    //         .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE
    //     };
    
    //     static VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
    //         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
    //         .pNext = nullptr, 
    //         .bufferDeviceAddress = VK_TRUE,
    //         .bufferDeviceAddressCaptureReplay = VK_FALSE,
    //         .bufferDeviceAddressMultiDevice = VK_FALSE
    //     };

    //     static VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{
    //         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
    //         .pNext = nullptr,
    //         .shaderInputAttachmentArrayDynamicIndexing = VK_FALSE,
    //         .shaderUniformTexelBufferArrayDynamicIndexing = VK_FALSE,
    //         .shaderStorageTexelBufferArrayDynamicIndexing = VK_FALSE,
    //         .shaderUniformBufferArrayNonUniformIndexing = VK_FALSE,
    //         .shaderSampledImageArrayNonUniformIndexing = VK_FALSE,
    //         .shaderStorageBufferArrayNonUniformIndexing = VK_FALSE,
    //         .shaderStorageImageArrayNonUniformIndexing = VK_FALSE,
    //         .shaderInputAttachmentArrayNonUniformIndexing = VK_FALSE,
    //         .shaderUniformTexelBufferArrayNonUniformIndexing = VK_FALSE,
    //         .shaderStorageTexelBufferArrayNonUniformIndexing = VK_FALSE,
    //         .descriptorBindingUniformBufferUpdateAfterBind = VK_FALSE,
    //         .descriptorBindingSampledImageUpdateAfterBind = VK_FALSE,
    //         .descriptorBindingStorageImageUpdateAfterBind = VK_FALSE,
    //         .descriptorBindingStorageBufferUpdateAfterBind = VK_FALSE,
    //         .descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE,
    //         .descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE,
    //         .descriptorBindingUpdateUnusedWhilePending = VK_FALSE,
    //         .descriptorBindingPartiallyBound = VK_TRUE,
    //         .descriptorBindingVariableDescriptorCount = VK_FALSE,
    //         .runtimeDescriptorArray = VK_TRUE
    //     };

    //     static VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalarBlockLayoutFeatures{
    //         .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT,
    //         .pNext = nullptr,
    //         .scalarBlockLayout = VK_TRUE,
    //     };
    
    //     // Link the pNext chain
    //     rayTracingPipelineFeatures.pNext = &accelStructureFeatures;
    //     accelStructureFeatures.pNext = &bufferDeviceAddressFeatures;
    //     bufferDeviceAddressFeatures.pNext = &descriptorIndexingFeatures;
    //     descriptorIndexingFeatures.pNext = &scalarBlockLayoutFeatures;
    
    //     // // Attach the chain to the features2 struct
    //     features2.pNext = &rayTracingPipelineFeatures;
    
    //     // You can also enable basic features like samplerAnisotropy here if needed:
    //     features2.features.samplerAnisotropy = VK_TRUE;
    //     features2.features.sampleRateShading = VK_TRUE;
    // });
    

    // QByteArrayList requiredDeviceExtensions = {
    //     VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    //     VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
    //     VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    //     VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    //     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    //     VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,  
    //     // VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    //     VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME     
    // };

    // this->setDeviceExtensions(requiredDeviceExtensions);

    this->setQueueCreateInfoModifier([this](const VkQueueFamilyProperties * /*queueFamilies*/,
                                            uint32_t /*queueFamilyCount*/,
                                            QList<VkDeviceQueueCreateInfo> &queueCreateInfos) {
        float priority = 1.0f;

        uint32_t computeQueueFamilyIndex = this->findQueueFamilyIndex(this->physicalDevice(), VK_QUEUE_COMPUTE_BIT);

        if (computeQueueFamilyIndex == UINT32_MAX) {
            qFatal("No suitable compute queue found");
        }

        // Check if we already requested this family
        bool alreadyRequested = false;
        for (auto &queueCreateInfo : queueCreateInfos) {
            if (queueCreateInfo.queueFamilyIndex == computeQueueFamilyIndex) {
                alreadyRequested = true;
                queueCreateInfo.queueCount = std::max(queueCreateInfo.queueCount, 1u);
                break;
            }
        }

        if (!alreadyRequested) {
            VkDeviceQueueCreateInfo computeQueueInfo{};
            computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            computeQueueInfo.queueFamilyIndex = computeQueueFamilyIndex;
            computeQueueInfo.queueCount = 1;
            computeQueueInfo.pQueuePriorities = &priority;

            queueCreateInfos.append(computeQueueInfo);
        }
    });
    
    QWindow::setCursor(Qt::OpenHandCursor);

    m_camera = new Camera(this);
    
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    m_vulkanRenderer = new VulkanRenderer(this);
    m_vulkanRayTracer = new VulkanRayTracer(this);

    return m_vulkanRenderer;
}

// void VulkanWindow::deviceCreated()
// {
//     // VkDevice device = this->device();

//     // QVulkanDeviceFunctions *deviceFunctions = this->vulkanInstance()->deviceFunctions(device);

//     // uint32_t computeQueueFamilyIndex = findQueueFamilyIndex(this->physicalDevice(), VK_QUEUE_GRAPHICS_BIT);
//     // if (computeQueueFamilyIndex == UINT32_MAX)
//     //     qDebug("No suitable compute queue family found!");

//     // deviceFunctions->vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &m_computeQueue);
// }

uint32_t VulkanWindow::findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits queueFlagBit)
{
    uint32_t queueFamilyCount = 0;
    this->vulkanInstance()->functions()->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    this->vulkanInstance()->functions()->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t fallbackIndex = UINT32_MAX; // Fallback if a dedicated queue isn't found

    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; queueFamilyIndex++) {
        if (queueFamilies[queueFamilyIndex].queueFlags & queueFlagBit) {
            if ((queueFlagBit == VK_QUEUE_COMPUTE_BIT) && !(queueFamilies[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                return queueFamilyIndex;  // Prefer a dedicated compute queue
            }
            if (fallbackIndex == UINT32_MAX) {
                fallbackIndex = queueFamilyIndex;  // Save first matching queue in case a dedicated one isn't found
            }
        }
    }
    
    return fallbackIndex;
}

VkShaderModule VulkanWindow::createShaderModule(const QString& filename)
{
    VkShaderModule shaderModule = VK_NULL_HANDLE;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        qWarning("Failed to read shader %s", qPrintable(filename));
        return shaderModule;
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

    QVulkanDeviceFunctions *deviceFunctions = this->vulkanInstance()->deviceFunctions(this->device());

    VkResult result = deviceFunctions->vkCreateShaderModule(this->device(), &shaderInfo, nullptr, &shaderModule);

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

void VulkanWindow::wheelEvent(QWheelEvent *event)
{
    // qDebug() << event->angleDelta().y();
    
    m_zoom = 1.0;
    
    if (event->angleDelta().y() > 0)
    {
        m_zoom = 0.9;
    }
    else
    {
        m_zoom = 1.1;
    }

    m_camera->cameraZoomUpdate(m_zoom);
}

void VulkanWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {
        m_lastCursorPosition = event->pos();  
        QWindow::setCursor(Qt::ClosedHandCursor);
    }
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event)
{
    static bool justWarped = false;

    if (justWarped) {
        justWarped = false;
        return;
    }

    QPoint globalPosition = mapToGlobal(event->pos());
    QRect windowRect = this->geometry();
    QPoint topLeft = mapToGlobal(QPoint(0, 0));
    int width = windowRect.width();
    int height = windowRect.height();

    QPoint newPosition = globalPosition;  // Default, no change

    // Check if we need to wrap
    if (globalPosition.x() >= topLeft.x() + width)
    {
        newPosition.setX(topLeft.x());  // Wrap to left
        justWarped = true;
    } 
    else if (globalPosition.x() < topLeft.x()) 
    {
        newPosition.setX(topLeft.x() + width - 1);  // Wrap to right
        justWarped = true;
    }
    if (globalPosition.y() >= topLeft.y() + height) 
    {
        newPosition.setY(topLeft.y());  // Wrap to top
        justWarped = true;
    } 
    else if (globalPosition.y() < topLeft.y()) 
    {
        newPosition.setY(topLeft.y() + height - 1);  // Wrap to bottom
        justWarped = true;
    }

    if (justWarped) {
        QCursor::setPos(newPosition);
        m_lastCursorPosition = mapFromGlobal(newPosition);  // **Update only after warp**
        return;
    }

    if (event->buttons() & Qt::LeftButton) 
    {
        m_deltaCursorPosition = event->pos() - m_lastCursorPosition;
        m_lastCursorPosition = event->pos();
        m_camera->cameraViewUpdate(m_deltaCursorPosition);
    }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {  
        QWindow::setCursor(Qt::OpenHandCursor);
    }
}

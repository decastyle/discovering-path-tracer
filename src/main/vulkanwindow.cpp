#include "vulkanwindow.h"
#include "vulkanrenderer.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QCursor>
#include <QObject>
#include "camera.h"




uint32_t VulkanWindow::findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit)
{
    uint32_t queueFamilyCount = 0;
    this->vulkanInstance()->functions()->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    this->vulkanInstance()->functions()->vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t fallbackIndex = UINT32_MAX; // Fallback if a dedicated queue isn't found

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & bit) {
            if ((bit == VK_QUEUE_COMPUTE_BIT) && !(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                return i;  // Prefer a dedicated compute queue
            }
            if (fallbackIndex == UINT32_MAX) {
                fallbackIndex = i;  // Save first matching queue in case a dedicated one isn't found
            }
        }
    }
    
    return fallbackIndex;
}

VulkanWindow::VulkanWindow()
{

    this->setEnabledFeaturesModifier([this](VkPhysicalDeviceFeatures2 &features2) {
        // Create feature structs with the required pNext chain
        static VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
            .pNext = nullptr,
            .rayTracingPipeline = VK_TRUE,
            .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
            .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
            .rayTracingPipelineTraceRaysIndirect = VK_FALSE,
            .rayTraversalPrimitiveCulling = VK_FALSE
        };
    
        static VkPhysicalDeviceAccelerationStructureFeaturesKHR accelStructureFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
            .pNext = nullptr,
            .accelerationStructure = VK_TRUE,
            .accelerationStructureCaptureReplay = VK_FALSE,
            .accelerationStructureIndirectBuild = VK_FALSE,
            .accelerationStructureHostCommands = VK_FALSE,
            .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE
        };
    
        static VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
            .pNext = nullptr, 
            .bufferDeviceAddress = VK_TRUE,
            .bufferDeviceAddressCaptureReplay = VK_FALSE,
            .bufferDeviceAddressMultiDevice = VK_FALSE
        };

        static VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{
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
            .descriptorBindingPartiallyBound = VK_TRUE,
            .descriptorBindingVariableDescriptorCount = VK_FALSE,
            .runtimeDescriptorArray = VK_TRUE
        };

        static VkPhysicalDeviceScalarBlockLayoutFeaturesEXT scalarBlockLayoutFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT,
            .pNext = nullptr,
            .scalarBlockLayout = VK_TRUE,
        };
    
        // Link the pNext chain
        rayTracingPipelineFeatures.pNext = &accelStructureFeatures;
        accelStructureFeatures.pNext = &bufferDeviceAddressFeatures;
        bufferDeviceAddressFeatures.pNext = &descriptorIndexingFeatures;
        descriptorIndexingFeatures.pNext = &scalarBlockLayoutFeatures;
    
        // // Attach the chain to the features2 struct
        features2.pNext = &rayTracingPipelineFeatures;
    
        // You can also enable basic features like samplerAnisotropy here if needed:
        features2.features.samplerAnisotropy = VK_TRUE;
        features2.features.sampleRateShading = VK_TRUE;
    });
    

    QByteArrayList requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,  
        // VK_KHR_SPIRV_1_4_EXTENSION_NAME,
        VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME     
    };

    this->setDeviceExtensions(requiredDeviceExtensions);




    
    QWindow::setCursor(Qt::OpenHandCursor);
    m_camera = new Camera(this);
    
    QObject::connect(this, &VulkanWindow::cameraViewUpdate, m_camera, &Camera::onCameraViewUpdate);
    QObject::connect(this, &VulkanWindow::cameraZoomUpdate, m_camera, &Camera::onCameraZoomUpdate);
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    







    m_renderer = new VulkanRenderer(this);
    m_raytracer = new VulkanRayTracer(this);

    QObject::connect(m_raytracer, &VulkanRayTracer::copySampledImage, m_renderer->m_helper, &VulkanRendererHelper::onCopySampledImageHelper);
    QObject::connect(m_renderer->m_helper, &VulkanRendererHelper::updateSwapChain, m_camera, &Camera::onUpdateSwapChain);

    return m_renderer;
}



void VulkanWindow::deviceCreated()
{
    VkDevice dev = this->device();

    QVulkanDeviceFunctions *devFuncs = this->vulkanInstance()->deviceFunctions(dev);
    
    VkSemaphoreCreateInfo semaphoreInfo
    { 
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    devFuncs->vkCreateSemaphore(dev, &semaphoreInfo, nullptr, &m_rayTracingFinishedSemaphore);
    devFuncs->vkCreateSemaphore(dev, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore);
    devFuncs->vkCreateSemaphore(dev, &semaphoreInfo, nullptr, &m_transferFinishedSemaphore);

    uint32_t computeQueueFamilyIndex = findQueueFamilyIndex(this->physicalDevice(), VK_QUEUE_GRAPHICS_BIT);
    if (computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");

    devFuncs->vkGetDeviceQueue(dev, computeQueueFamilyIndex, 0, &m_computeQueue);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = nullptr,
        .commandBufferCount = 0,
        .pCommandBuffers = nullptr,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_transferFinishedSemaphore
    };

    devFuncs->vkQueueSubmit(m_computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    devFuncs->vkQueueWaitIdle(m_computeQueue);  // Optional, depends on your needs

    m_submissionManager = new VulkanSubmissionManager(dev, m_computeQueue);
}

VulkanRayTracer *VulkanWindow::getVulkanRayTracer()
{
    return m_raytracer;
}


VulkanRenderer *VulkanWindow::getVulkanRenderer()
{
    return m_renderer;
}

Camera *VulkanWindow::getCamera()
{
    return m_camera;
}

void VulkanWindow::wheelEvent(QWheelEvent *event)
{
    qDebug() << event->angleDelta().y();
    
    m_zoom = 1.0;
    
    if (event->angleDelta().y() > 0)
    {
        m_zoom = 0.9;
    }
    else
    {
        m_zoom = 1.1;
    }

    emit cameraZoomUpdate(m_zoom);
}



void VulkanWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {
        m_lastCursorPos = event->pos();  
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

    QPoint globalPos = mapToGlobal(event->pos());
    QRect windowRect = this->geometry();
    QPoint topLeft = mapToGlobal(QPoint(0, 0));
    int width = windowRect.width();
    int height = windowRect.height();

    QPoint newPos = globalPos;  // Default, no change

    // Check if we need to wrap
    if (globalPos.x() >= topLeft.x() + width) {
        newPos.setX(topLeft.x());  // Wrap to left
        justWarped = true;
    } else if (globalPos.x() < topLeft.x()) {
        newPos.setX(topLeft.x() + width - 1);  // Wrap to right
        justWarped = true;
    }
    if (globalPos.y() >= topLeft.y() + height) {
        newPos.setY(topLeft.y());  // Wrap to top
        justWarped = true;
    } else if (globalPos.y() < topLeft.y()) {
        newPos.setY(topLeft.y() + height - 1);  // Wrap to bottom
        justWarped = true;
    }

    if (justWarped) {
        QCursor::setPos(newPos);
        m_lastCursorPos = mapFromGlobal(newPos);  // **Update only after warp**
        return;
    }

    if (event->buttons() & Qt::LeftButton) 
    {
        m_delta = event->pos() - m_lastCursorPos;
        m_lastCursorPos = event->pos();
        emit cameraViewUpdate(m_delta);
    }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {  
        QWindow::setCursor(Qt::OpenHandCursor);
    }
}


VkSemaphore *VulkanWindow::getRayTracingFinishedSemaphore()
{
    return &m_rayTracingFinishedSemaphore;
}
VkSemaphore *VulkanWindow::getRenderFinishedSemaphore()
{
    return &m_renderFinishedSemaphore;
}

VkSemaphore *VulkanWindow::getTransferFinishedSemaphore()
{
    return &m_transferFinishedSemaphore;
}


std::mutex *VulkanWindow::getQueueMutex()
{
    return &queueMutex;
}
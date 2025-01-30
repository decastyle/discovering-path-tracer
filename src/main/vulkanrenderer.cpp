#include "vulkanrenderer.h"
#include "vulkanwindow.h"
#include <QVulkanFunctions>
#include <QFile>

VulkanRenderer::VulkanRenderer(VulkanWindow *w)
    : m_window(w)
{
}

VkShaderModule VulkanRenderer::createShaderModule(const QString& filename)
{
    VkShaderModule shaderModule;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(filename));
        shaderModule = VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo;
    memset(&shaderInfo, 0, sizeof(shaderInfo));
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = blob.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t *>(blob.constData());
    VkResult err = m_devFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo, nullptr, &shaderModule);
    if (err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d", err);
        shaderModule = VK_NULL_HANDLE;
    }
    else{
        qDebug("Shaders loaded successfully!");
    }
    return shaderModule;
}

void VulkanRenderer::initResources()
{
    QVulkanInstance *inst = m_window->vulkanInstance();
    VkDevice dev = m_window->device();
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);

    VkShaderModule vertShaderModule = createShaderModule(QStringLiteral(":/color_vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(QStringLiteral(":/color_frag.spv"));

    if (vertShaderModule)
        m_devFuncs->vkDestroyShaderModule(dev, vertShaderModule, nullptr);
    if (fragShaderModule)
        m_devFuncs->vkDestroyShaderModule(dev, fragShaderModule, nullptr);


    // Logs

    QString info;
    info += QString::asprintf("Number of physical devices: %d\n", int(m_window->availablePhysicalDevices().count()));

    QVulkanFunctions *f = inst->functions();
    VkPhysicalDeviceProperties props;
    f->vkGetPhysicalDeviceProperties(m_window->physicalDevice(), &props);
    info += QString::asprintf("Active physical device name: '%s' version %d.%d.%d\nAPI version %d.%d.%d\n",
                              props.deviceName,
                              VK_VERSION_MAJOR(props.driverVersion), VK_VERSION_MINOR(props.driverVersion),
                              VK_VERSION_PATCH(props.driverVersion),
                              VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
                              VK_VERSION_PATCH(props.apiVersion));

    info += QStringLiteral("Supported instance layers:\n");
    for (const QVulkanLayer &layer : inst->supportedLayers())
        info += QString::asprintf("    %s v%u\n", layer.name.constData(), layer.version);
    info += QStringLiteral("Enabled instance layers:\n");
    for (const QByteArray &layer : inst->layers())
        info += QString::asprintf("    %s\n", layer.constData());

    info += QStringLiteral("Supported instance extensions:\n");
    for (const QVulkanExtension &ext : inst->supportedExtensions())
        info += QString::asprintf("    %s v%u\n", ext.name.constData(), ext.version);
    info += QStringLiteral("Enabled instance extensions:\n");
    for (const QByteArray &ext : inst->extensions())
        info += QString::asprintf("    %s\n", ext.constData());

    info += QString::asprintf("Color format: %u\nDepth-stencil format: %u\n",
                              m_window->colorFormat(), m_window->depthStencilFormat());

    info += QStringLiteral("Supported sample counts:");
    const QList<int> sampleCounts = m_window->supportedSampleCounts();
    for (int count : sampleCounts)
        info += QLatin1Char(' ') + QString::number(count);
    info += QLatin1Char('\n');

    emit static_cast<VulkanWindow *>(m_window)->vulkanInfoReceived(info);
}

void VulkanRenderer::startNextFrame()
{
    m_hue += 0.005f;
    if (m_hue > 1.0f) {
        m_hue = 0.0f;
    }
    QColor color = QColor::fromHslF(m_hue, 0.5, 0.5);

    VkClearValue clearValues[2]{};
    clearValues[0].color = {
        color.redF(),
        color.greenF(),
        color.blueF(),
        1.0f
    };
    clearValues[1].depthStencil = { 
        1.0f, 
        0
    };

    const QSize imageSize = m_window->swapChainImageSize();

    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_window->defaultRenderPass();
    beginInfo.framebuffer = m_window->currentFramebuffer();
    beginInfo.renderArea.extent.width = imageSize.width();
    beginInfo.renderArea.extent.height = imageSize.height();
    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearValues;

    VkCommandBuffer commandBuffer = m_window->currentCommandBuffer();
    m_devFuncs->vkCmdBeginRenderPass(commandBuffer, &beginInfo,
                                     VK_SUBPASS_CONTENTS_INLINE);
    m_devFuncs->vkCmdEndRenderPass(commandBuffer);
    m_window->frameReady();
    /*
    Each call to this function must be followed by a call to QVulkanWindow::frameReady(). Failing to do so will stall the rendering loop. The call can also be made at a later time, after returning from this function. This means that it is possible to kick off asynchronous work, and only update the command buffer and notify QVulkanWindow when that work has finished.
    */
    m_window->requestUpdate();

    emit static_cast<VulkanWindow *>(m_window)->frameQueued();
}

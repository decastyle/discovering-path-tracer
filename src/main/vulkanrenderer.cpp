#include "vulkanrenderer.h"
#include <QVulkanFunctions>
#include <QFile>

VulkanRenderer::VulkanRenderer(QVulkanWindow *w)
    : m_window(w)
{
}

void VulkanRenderer::initResources()
{
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(m_window->device());
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
}

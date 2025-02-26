#include "VulkanCommandBuffer.h"
#include "VulkanWindow.h"

VulkanCommandBuffer::VulkanCommandBuffer(VulkanWindow* vulkanWindow, VkCommandPool commandPool, VkQueue queue)
    : m_vulkanWindow(vulkanWindow), 
      m_commandPool(commandPool), 
      m_queue(queue)
{
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_vulkanWindow->device());

    createCommandBuffer();
    createFence();
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    cleanup();
}

void VulkanCommandBuffer::createCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo 
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = m_commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    m_result = m_deviceFunctions->vkAllocateCommandBuffers(m_vulkanWindow->device(), &commandBufferAllocateInfo, &m_commandBuffer);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to allocate command buffer (error code: %d)", m_result);
        return;
    }
}

void VulkanCommandBuffer::createFence()
{
    VkFenceCreateInfo fenceCreateInfo
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    m_result = m_deviceFunctions->vkCreateFence(m_vulkanWindow->device(), &fenceCreateInfo, nullptr, &m_fence);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create fence (error code: %d)", m_result);
        return;
    }
}

void VulkanCommandBuffer::cleanup()
{
    if (m_commandBuffer != VK_NULL_HANDLE)
    {
        m_deviceFunctions->vkFreeCommandBuffers(m_vulkanWindow->device(), m_commandPool, 1, &m_commandBuffer);
        m_commandBuffer = VK_NULL_HANDLE;
    }
    if (m_fence != VK_NULL_HANDLE)
    {
        m_deviceFunctions->vkDestroyFence(m_vulkanWindow->device(), m_fence, nullptr);
        m_fence = VK_NULL_HANDLE;
    }
}

void VulkanCommandBuffer::beginSingleTimeCommandBuffer()
{
    if (m_isRecording)
    {
        qWarning("Command buffer is already recording!");
        return;
    }

    VkCommandBufferBeginInfo commandBufferBeginInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };
    
    m_result = m_deviceFunctions->vkBeginCommandBuffer(m_commandBuffer, &commandBufferBeginInfo);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to begin command buffer (error code: %d)", m_result);
        return;
    }

    m_isRecording = true;
}

void VulkanCommandBuffer::endSubmitAndWait()
{
    if (!m_isRecording)
    {
        qWarning("Attempted to end a command buffer that is not recording!");
        return;
    }

    m_result = m_deviceFunctions->vkEndCommandBuffer(m_commandBuffer);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to end command buffer (error code: %d)", m_result);
        return;
    }

    m_isRecording = false;

    VkSubmitInfo submitInfo 
    {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = nullptr,
        .waitSemaphoreCount   = 0,
        .pWaitSemaphores      = nullptr,
        .pWaitDstStageMask    = nullptr,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &m_commandBuffer,
        .signalSemaphoreCount = 0,
        .pSignalSemaphores    = nullptr
    };   

    m_result = m_deviceFunctions->vkResetFences(m_vulkanWindow->device(), 1, &m_fence);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to reset fence (error code: %d)", m_result);
        return;
    }

    m_result = m_deviceFunctions->vkQueueSubmit(m_queue, 1, &submitInfo, m_fence);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to submit command buffer to queue (error code: %d)", m_result);
        return;
    }

    m_result = m_deviceFunctions->vkWaitForFences(m_vulkanWindow->device(), 1, &m_fence, VK_TRUE, UINT64_MAX);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to wait for fence (error code: %d)", m_result);
        return;
    }
}
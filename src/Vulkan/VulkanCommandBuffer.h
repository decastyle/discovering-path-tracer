#pragma once

#include <vulkan/vulkan.h>
#include <QVulkanDeviceFunctions>
#include <vector>

class VulkanWindow;

class VulkanCommandBuffer
{
public:
    VulkanCommandBuffer() = default;
    VulkanCommandBuffer(VulkanWindow* vulkanWindow, VkCommandPool commandPool, VkQueue queue);
    ~VulkanCommandBuffer();

    void beginSingleTimeCommandBuffer();

    // End recording of the command buffer
    void endRecording();

    // Submit the command buffer with optional semaphores (uses stored semaphores if not provided)
    void submit(const std::vector<VkSemaphore>& waitSemaphores = {},
                const std::vector<VkPipelineStageFlags>& waitStageMasks = {},
                const std::vector<VkSemaphore>& signalSemaphores = {});

    // Wait for the command buffer to complete execution
    void wait();

    // End recording, submit, and wait (convenience method)
    void endSubmitAndWait(const std::vector<VkSemaphore>& waitSemaphores = {},
                          const std::vector<VkPipelineStageFlags>& waitStageMasks = {},
                          const std::vector<VkSemaphore>& signalSemaphores = {});

    // Getters and setters for stored semaphores
    void setWaitSemaphores(const std::vector<VkSemaphore>& semaphores,
                          const std::vector<VkPipelineStageFlags>& stageMasks);
    void setSignalSemaphores(const std::vector<VkSemaphore>& semaphores);
    std::vector<VkSemaphore> waitSemaphores() const { return m_waitSemaphores; }
    std::vector<VkPipelineStageFlags> waitStageMasks() const { return m_waitStageMasks; }
    std::vector<VkSemaphore> signalSemaphores() const { return m_signalSemaphores; }

    VkCommandBuffer getCommandBuffer() const { return m_commandBuffer; }

    VkFence getFence() const { return m_fence; }

    void destroy() { cleanup(); }

private:
    void createCommandBuffer();
    void createFence();
    void cleanup();

    VulkanWindow* m_vulkanWindow = nullptr;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;

    // Stored semaphores for reuse or external access
    std::vector<VkSemaphore> m_waitSemaphores;
    std::vector<VkPipelineStageFlags> m_waitStageMasks;
    std::vector<VkSemaphore> m_signalSemaphores;

    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    bool m_isRecording = false;

    VkResult m_result = VK_NOT_READY;
    QVulkanDeviceFunctions* m_deviceFunctions = nullptr;
};
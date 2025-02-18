#ifndef VULKANSUBMISSIONMANAGER_H
#define VULKANSUBMISSIONMANAGER_H

#include <vulkan/vulkan.h>
#include <vector>
#include <mutex>

class VulkanSubmissionManager {
public:
    VulkanSubmissionManager(VkDevice device, VkQueue queue);
    ~VulkanSubmissionManager();

    void addCommandBuffer(VkCommandBuffer cmdBuf, VkPipelineStageFlags waitStage, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore);
    void submit();

private:
    std::mutex m_mutex{};
    VkDevice m_device;
    VkQueue m_queue;

    struct Submission 
    {
        VkCommandBuffer cmdBuf;
        VkPipelineStageFlags waitStage;
        VkSemaphore waitSemaphore;
        VkSemaphore signalSemaphore;
    };

    std::vector<Submission> m_submissions;
};

#endif // VULKANSUBMISSIONMANAGER_H

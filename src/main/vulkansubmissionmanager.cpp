#include "VulkanSubmissionManager.h"
#include <QThread>
#include <QDebug>


VulkanSubmissionManager::VulkanSubmissionManager(VkDevice device, VkQueue queue)
    : m_device(device), m_queue(queue) {}

VulkanSubmissionManager::~VulkanSubmissionManager() {}

void VulkanSubmissionManager::addCommandBuffer(VkCommandBuffer cmdBuf, VkPipelineStageFlags waitStage, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_submissions.push_back({ cmdBuf, waitStage, waitSemaphore, signalSemaphore });

    qDebug() << "void VulkanSubmissionManager::addCommandBuffer running in thread:" << QThread::currentThread();
}

void VulkanSubmissionManager::submit() 
{
    std::lock_guard<std::mutex> lock(m_mutex);  

    if (m_submissions.empty()) return;

    std::vector<VkSubmitInfo> submitInfos;
    std::vector<VkFence> fences(m_submissions.size());
    
    for (size_t i = 0; i < m_submissions.size(); ++i) {
        auto& sub = m_submissions[i];

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &sub.cmdBuf;

        // Use per-submit semaphore storage
        VkSemaphore waitSemaphores[] = { sub.waitSemaphore };
        VkPipelineStageFlags waitStages[] = { sub.waitStage };
        VkSemaphore signalSemaphores[] = { sub.signalSemaphore };

        if (sub.waitSemaphore != VK_NULL_HANDLE) {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
        }

        if (sub.signalSemaphore != VK_NULL_HANDLE) {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;
        }

        submitInfos.push_back(submitInfo);
    }

     // Create a fence to ensure safe clearing of m_submissions
     VkFenceCreateInfo fenceInfo{};
     fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
     VkFence submitFence;
     vkCreateFence(m_device, &fenceInfo, nullptr, &submitFence);
 
     if (vkQueueSubmit(m_queue, submitInfos.size(), submitInfos.data(), submitFence) != VK_SUCCESS) {
         qDebug() << "Failed to submit command buffers!";
     } else {
         // Wait for execution to finish before clearing the list
         vkWaitForFences(m_device, 1, &submitFence, VK_TRUE, UINT64_MAX);
         vkDestroyFence(m_device, submitFence, nullptr);
         m_submissions.clear();
     }
}

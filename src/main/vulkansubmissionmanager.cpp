#include "VulkanSubmissionManager.h"
#include <QThread>
#include <QDebug>

VulkanSubmissionManager::VulkanSubmissionManager(VkDevice device, VkQueue queue)
    : m_device(device), m_queue(queue) {}

VulkanSubmissionManager::~VulkanSubmissionManager() {}

void VulkanSubmissionManager::addCommandBuffer(VkCommandBuffer cmdBuf, VkPipelineStageFlags waitStage, VkSemaphore waitSemaphore, VkSemaphore signalSemaphore) {
    m_submissions.push_back({ cmdBuf, waitStage, waitSemaphore, signalSemaphore });
    qDebug() << "void VulkanSubmissionManager::addCommandBuffer running in thread:" << QThread::currentThread();
}

void VulkanSubmissionManager::submit() {
    if (m_submissions.empty()) return;

    std::vector<VkSubmitInfo> submitInfos;
    // std::vector<VkSemaphore> waitSemaphores;
    // std::vector<VkSemaphore> signalSemaphores;
    // std::vector<VkPipelineStageFlags> waitStages;

    for (auto& sub : m_submissions) {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &sub.cmdBuf;

        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;

        if (sub.waitSemaphore != VK_NULL_HANDLE) {
            waitSemaphores.push_back(sub.waitSemaphore);
            waitStages.push_back(sub.waitStage);
        }

        if (sub.signalSemaphore != VK_NULL_HANDLE) {
            signalSemaphores.push_back(sub.signalSemaphore);
        }

        submitInfo.waitSemaphoreCount = waitSemaphores.size();
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.signalSemaphoreCount = signalSemaphores.size();
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        submitInfos.push_back(submitInfo);
    }

    vkQueueSubmit(m_queue, submitInfos.size(), submitInfos.data(), VK_NULL_HANDLE);
    m_submissions.clear();
}

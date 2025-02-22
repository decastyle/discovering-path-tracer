#include "VulkanCommandPool.h"

VulkanCommandPool::VulkanCommandPool(QVulkanWindow* vulkanWindow, uint32_t queueFamilyIndex)
    : m_vulkanWindow(vulkanWindow), 
      m_queueFamilyIndex(queueFamilyIndex)
{
    m_device = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_device);

    createCommandPool();
}

VulkanCommandPool::~VulkanCommandPool()
{
    cleanup();
}

void VulkanCommandPool::createCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo 
    {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = m_queueFamilyIndex
    };
    
    VkCommandPool commandPool;

    m_result = m_deviceFunctions->vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &commandPool);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create command pool (error code: %d)", m_result);
        return;
    }
}

void VulkanCommandPool::cleanup()
{
    if (m_commandPool)
    {
        m_deviceFunctions->vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}
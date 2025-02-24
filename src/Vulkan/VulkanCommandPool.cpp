#include "VulkanCommandPool.h"
#include "VulkanWindow.h"
#include <QVulkanFunctions>

VulkanCommandPool::VulkanCommandPool(VulkanWindow* vulkanWindow, uint32_t queueFamilyIndex)
    : m_vulkanWindow(vulkanWindow), 
      m_queueFamilyIndex(queueFamilyIndex)
{
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_vulkanWindow->device());

    createCommandPool();
}

VulkanCommandPool::~VulkanCommandPool()
{
    cleanup();
}

void VulkanCommandPool::swap(VulkanCommandPool& other) noexcept
{
    std::swap(m_vulkanWindow, other.m_vulkanWindow);
    std::swap(m_queueFamilyIndex, other.m_queueFamilyIndex);
    
    // Vulkan resources
    std::swap(m_commandPool, other.m_commandPool);
    
    // Device resources
    std::swap(m_result, other.m_result);
    std::swap(m_deviceFunctions, other.m_deviceFunctions);
}

VulkanCommandPool& VulkanCommandPool::operator=(VulkanCommandPool&& other) noexcept 
{
    if (this != &other) 
    {
        cleanup();
        swap(other);
    }
    return *this;
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

    m_result = m_deviceFunctions->vkCreateCommandPool(m_vulkanWindow->device(), &commandPoolCreateInfo, nullptr, &m_commandPool);
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
        m_deviceFunctions->vkDestroyCommandPool(m_vulkanWindow->device(), m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
}
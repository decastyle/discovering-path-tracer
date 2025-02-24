#pragma once

#include <QVulkanWindow>

#include "VulkanRayTracer.h"
#include "VulkanRenderer.h"
#include "Camera.h"

class VulkanWindow : public QVulkanWindow
{
public:
    VulkanWindow();
    
    QVulkanWindowRenderer* createRenderer() override;

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VkShaderModule createShaderModule(const QString& filename);

    Camera* getCamera() { return m_camera; }
    VulkanRayTracer* getVulkanRayTracer() { return m_vulkanRayTracer; }
    VulkanRenderer* getVulkanRenderer() { return m_vulkanRenderer; }

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    QPoint m_deltaCursorPosition{};
    float m_zoom{};
    QPoint m_lastCursorPosition{};

    Camera* m_camera = nullptr;
    VulkanRayTracer* m_vulkanRayTracer = nullptr;
    VulkanRenderer* m_vulkanRenderer = nullptr;
};
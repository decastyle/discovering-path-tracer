#pragma once

#include <QObject>
#include <QVulkanWindow>
#include <QString>
#include <QWindow>
#include <QCursor>
#include <QWheelEvent>
#include <QMouseEvent>
#include <vulkan/vulkan.h>

#include "Camera.h"
#include "VulkanRenderer.h"
#include "VulkanRayTracer.h"

class Camera;
class VulkanRenderer;
class VulkanRayTracer;

class VulkanWindow : public QVulkanWindow
{
public:
    VulkanWindow();
    
    QVulkanWindowRenderer *createRenderer() override;

    void deviceCreated();

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VkShaderModule createShaderModule(const QString &filename);

    Camera* getCamera() { return m_camera; }
    VulkanRayTracer* getVulkanRayTracer() { return m_vulkanRayTracer; }
    VulkanRenderer* getVulkanRenderer() { return m_vulkanRenderer; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QPoint m_deltaCursorPosition{};
    float m_zoom{};
    QPoint m_lastCursorPosition{};

    Camera *m_camera;
    VulkanRayTracer *m_vulkanRayTracer;
    VulkanRenderer *m_vulkanRenderer;
};

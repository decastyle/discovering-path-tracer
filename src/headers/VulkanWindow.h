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

    Camera *getCamera();
    VulkanRayTracer *getVulkanRayTracer();
    VulkanRenderer *getVulkanRenderer();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    Camera *m_camera;
    VulkanRayTracer *m_vulkanRayTracer;
    VulkanRenderer *m_vulkanRenderer;

    QPoint m_deltaCursorPosition{};
    float m_zoom{};
    QPoint m_lastCursorPosition{};
};

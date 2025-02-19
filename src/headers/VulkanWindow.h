#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QObject>
#include <QVulkanWindow>
#include <QString>
#include <QWindow>
#include <QCursor>
#include <vulkan/vulkan.h>


#include "Camera.h"
#include "VulkanRenderer.h"
#include "VulkanRayTracer.h"


class Camera;
class VulkanRenderer;
class VulkanRayTracer;

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT
public:
    VulkanWindow();
    
    QVulkanWindowRenderer *createRenderer() override;

    void deviceCreated();

    Camera *getCamera();
    VulkanRayTracer *getVulkanRayTracer();
    VulkanRenderer *getVulkanRenderer();

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);
    VkShaderModule createShaderModule(const QString &filename);

signals:
    void cameraViewUpdate(QPoint m_delta);
    void cameraZoomUpdate(float m_zoom);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    VkQueue m_computeQueue;

    Camera *m_camera;
    VulkanRayTracer *m_rayTracer;
    VulkanRenderer *m_renderer;

    QPoint m_delta{};
    float m_zoom{};
    QPoint m_lastCursorPos{};
};

#endif // VULKANWINDOW_H
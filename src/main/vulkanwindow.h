#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QObject>
#include <QVulkanWindow>
#include <QString>
#include <QWindow>
#include <QCursor>
#include "camera.h"
#include "vulkanrenderer.h"
#include "vulkanraytracer.h"
#include <vulkan/vulkan.h>
#include "vulkansubmissionmanager.h"

class Camera;

class VulkanRenderer;

class VulkanRayTracer;
class VulkanSubmissionManager;

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    VulkanWindow();
    
    QVulkanWindowRenderer *createRenderer() override;

    Camera *getCamera();

    void deviceCreated();

    VulkanRayTracer *getVulkanRayTracer();
    VulkanRenderer *getVulkanRenderer();
    VkSemaphore *getRayTracingFinishedSemaphore();
    VkSemaphore *getRenderFinishedSemaphore();
    VkSemaphore *getTransferFinishedSemaphore();

    VulkanSubmissionManager* m_submissionManager;

    std::mutex *getQueueMutex();

signals:
    void vulkanInfoReceived(const QString &text);
    void cameraViewUpdate(QPoint m_delta);
    void cameraZoomUpdate(float m_zoom);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    std::mutex queueMutex{}; 

    VkQueue m_computeQueue;
    VkQueue m_graphicsQueue;

    uint32_t findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkQueueFlagBits bit);

    // VkSemaphore m_renderFinishedSemaphore;
    // VkSemaphore m_rayTracingFinishedSemaphore;
    // VkSemaphore m_transferFinishedSemaphore;

    VkDevice m_device;
    Camera *m_camera;
    VulkanRayTracer *m_raytracer;
    VulkanRenderer *m_renderer;

    QPoint m_delta{};
    float m_zoom{};
    QPoint m_lastCursorPos{};
};

#endif // VULKANWINDOW_H
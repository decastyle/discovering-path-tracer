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

class Camera;

class VulkanRenderer;

class VulkanRayTracer;

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    VulkanWindow();
    
    QVulkanWindowRenderer *createRenderer() override;

    Camera *getCamera();

    VulkanRayTracer *getVulkanRayTracer();

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
    Camera *m_camera;
    VulkanRayTracer *m_raytracer;
    VulkanRenderer *m_renderer;

    QPoint m_delta{};
    float m_zoom{};
    QPoint m_lastCursorPos{};
};

#endif // VULKANWINDOW_H
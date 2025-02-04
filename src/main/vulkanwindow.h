#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QObject>
#include <QVulkanWindow>
#include <QString>
#include <QWindow>
#include <QCursor>
#include "camera.h"
#include "vulkanrenderer.h"

class Camera;

class VulkanRenderer;

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    VulkanWindow();
    
    QVulkanWindowRenderer *createRenderer() override;

    Camera *getCamera();

signals:
    void vulkanInfoReceived(const QString &text);
    void frameQueued();
    void cameraViewUpdate(QPoint m_delta);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Camera *m_camera;
    VulkanRenderer *m_renderer;

    QPoint m_delta{};
    QPoint m_lastCursorPos{};
};

#endif // VULKANWINDOW_H
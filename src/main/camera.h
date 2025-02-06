#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QVector3D>
#include <QVulkanWindow>
#include "vulkanwindow.h"

class VulkanWindow;

class Camera : public QObject
{
    Q_OBJECT

public:
    Camera(VulkanWindow *w);

    QMatrix4x4 getProj();
    QVector3D getPos();

public slots:
    void onCameraViewUpdate(QPoint m_delta);
    void onUpdateSwapChain();

private:
    void updateProjection();
    VulkanWindow *m_window;

    QVector3D m_cameraPos{};    

    QQuaternion m_rotation{};
    QMatrix4x4 m_proj{}; 
    QMatrix4x4 m_updatedMatrix{};

    float m_sensitivity;
    float m_fov;      
    float m_radius;
    float m_yaw, m_pitch;
};

#endif // CAMERA_H
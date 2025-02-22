#pragma once

#include <QObject>
#include <QVector3D>
#include <QVulkanWindow>

#include "VulkanWindow.h"

class VulkanWindow;

class Camera
{
public:
    Camera(VulkanWindow *w);

    QMatrix4x4 getProjectionMatrix();
    QVector3D getPosition();

    void cameraSwapChainUpdate();
    void cameraViewUpdate(QPoint m_deltaCursorPosition);
    void cameraZoomUpdate(float m_zoom);

private:
    VulkanWindow *m_vulkanWindow;

    QVector3D m_cameraPosition{};    

    QQuaternion m_rotation{};
    QMatrix4x4 m_projectionMatrix{}; 
    QMatrix4x4 m_updatedMatrix{};

    float m_fov = 60.0f;
    float m_sensitivity = 0.25f;
    float m_radius = 0.9f;  
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
};

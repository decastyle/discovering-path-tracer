#pragma once

#include <QVector3D>
#include <QMatrix4x4>
#include <QQuaternion>

class VulkanWindow;

class Camera 
{
public:
    Camera(VulkanWindow* w);

    QMatrix4x4 getProjectionMatrix() const;

    QVector3D getPosition() const;
    QVector3D getDirection() const; 
    QVector3D getUp() const;       
    float getFov() const;  

    void cameraSwapChainUpdate();
    void cameraViewUpdate(QPoint m_deltaCursorPosition);
    void cameraZoomUpdate(float m_zoom);

private:
    VulkanWindow* m_vulkanWindow = nullptr;

    QVector3D m_cameraPosition{};    

    QQuaternion m_rotation{};
    QMatrix4x4 m_projectionMatrix{};
    QMatrix4x4 m_updatedMatrix{};

    float m_fov = 60.0f;
    float m_sensitivity = 0.25f;
    float m_radius = 5.0f;
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
};
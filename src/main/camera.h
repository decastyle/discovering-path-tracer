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

public slots:
    void onCameraAxisUpdate();
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
    
    
    // TODO: Create camera Q_OBJECT and link camera parameters (translate, FOV, rotation) to m_proj
    // Connect mouse wheel to zoom, GUI scroll to FOV, and rotate camera only when Alt is pressed
    // m_proj is used to drive rasterization view
    // In case of raytracing, camera parameters will be used to define ray's spawn point
};

#endif // CAMERA_H
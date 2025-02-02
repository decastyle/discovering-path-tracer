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

private:
    void updateProjection();
    VulkanWindow *m_window;

    QVector3D m_originPos{0.0f, 0.0f, 0.0f}; // Triangle is at the origin
    QVector3D m_originUp{0.0f, 1.0f, 0.0f};  // World up
    QVector3D m_originRight{};   
    QVector3D m_originForward{};

    QVector3D m_cameraSpawnPos{};
    QVector3D m_cameraPos{};    
    QVector3D m_cameraRot{};

    QMatrix4x4 m_rotation{};
    QMatrix4x4 m_proj{}; 
    QMatrix4x4 m_updatedMatrix{};

    float m_sensitivity;
    float m_fov;      
    
    
    // TODO: Create camera Q_OBJECT and link camera parameters (translate, FOV, rotation) to m_proj
    // Connect mouse wheel to zoom, GUI scroll to FOV, and rotate camera only when Alt is pressed
    // m_proj is used to drive rasterization view
    // In case of raytracing, camera parameters will be used to define ray's spawn point
};

#endif // CAMERA_H
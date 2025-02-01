#ifndef CAMERA_H
#define CAMERA_H

#include <QObject>
#include <QVector3D>

class Camera
{
    Q_OBJECT

public:
    Camera();

public slots:
    void onMouseClicked();

private:
    QVector3D m_rotationAxis{};   

    QVector3D m_translate{};    
    QVector3D m_rotation{};
    float m_fov;      
    
    
    // TODO: Create camera Q_OBJECT and link camera parameters (translate, FOV, rotation) to m_proj
    // Connect mouse wheel to zoom, GUI scroll to FOV, and rotate camera only when Alt is pressed
    // m_proj is used to drive rasterization view
    // In case of raytracing, camera parameters will be used to define ray's spawn point
};

#endif // VULKANRENDERER_H
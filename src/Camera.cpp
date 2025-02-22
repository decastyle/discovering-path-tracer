#include "Camera.h"

Camera::Camera(VulkanWindow *w) 
    : m_vulkanWindow(w)
{
    QVector3D offset(0, 0, m_radius);
    m_cameraPosition = m_rotation.rotatedVector(offset);
    m_updatedMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
}

void Camera::cameraSwapChainUpdate() 
{
    QSize swapChainSize = m_vulkanWindow->swapChainImageSize();
    float aspectRatio = swapChainSize.width() / static_cast<float>(swapChainSize.height());

    m_projectionMatrix.setToIdentity();
    m_projectionMatrix = m_vulkanWindow->clipCorrectionMatrix();
    
    m_projectionMatrix.perspective(m_fov, aspectRatio, 0.01f, 100.0f);


    // m_projectionMatrix.setToIdentity();
    // m_projectionMatrix = m_vulkanWindow->clipCorrectionMatrix();

    // float orthoSize = 5.0f; // Controls how much of the scene is visible
    // float left   = -orthoSize * aspectRatio;
    // float right  =  orthoSize * aspectRatio;
    // float bottom = -orthoSize;
    // float top    =  orthoSize;
    // float zNear  =  0.01f;
    // float zFar   =  100.0f;

    // m_projectionMatrix.ortho(left, right, bottom, top, zNear, zFar);
}

void Camera::cameraViewUpdate(QPoint m_deltaCursorPosition) // TODO: on middle click update origin
{
    static int correction = -1;
    m_yaw += (m_deltaCursorPosition.x() * correction) * m_sensitivity;
    m_pitch += (m_deltaCursorPosition.y() * -1) * m_sensitivity;

    QQuaternion yawQuaternion = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), m_yaw);
    QQuaternion pitchQuaternion = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), m_pitch);

    m_rotation = yawQuaternion * pitchQuaternion;

    QVector3D offset(0, 0, m_radius);
    m_cameraPosition = m_rotation.rotatedVector(offset);

    QVector3D cameraUp = m_rotation * QVector3D(0, 1, 0);

    m_updatedMatrix.setToIdentity();
    m_updatedMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), cameraUp);

    if(cameraUp.y() < 0)
    {
        correction = 1;
    }
    else
    {
        correction = -1;
    }
}

void Camera::cameraZoomUpdate(float m_zoom)
{
    m_radius *= m_zoom;
    
    QVector3D offset(0, 0, m_radius);
    m_cameraPosition = m_rotation.rotatedVector(offset);

    QVector3D cameraUp = m_rotation * QVector3D(0, 1, 0);

    m_updatedMatrix.setToIdentity();
    m_updatedMatrix.lookAt(m_cameraPosition, QVector3D(0, 0, 0), cameraUp);
}

QMatrix4x4 Camera::getProjectionMatrix()
{
    return m_projectionMatrix * m_updatedMatrix;
}

QVector3D Camera::getPosition()
{
    return m_cameraPosition;
}
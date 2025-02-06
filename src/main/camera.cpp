#include "camera.h"
#include "vulkanwindow.h" 

Camera::Camera(VulkanWindow *w) : m_window(w)
{
    m_fov = 45.0f;
    m_sensitivity = 0.25f;
    m_radius = 5.0f;  
    m_yaw = 0.0f;
    m_pitch = 0.0f;

    QVector3D offset(0, 0, m_radius);
    m_cameraPos = m_rotation.rotatedVector(offset);
    m_updatedMatrix.lookAt(m_cameraPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
}

void Camera::onUpdateSwapChain()
{
    updateProjection();
}

void Camera::updateProjection() {
    const QSize sz = m_window->swapChainImageSize();
    float aspectRatio = sz.width() / static_cast<float>(sz.height());

    m_proj.setToIdentity();
    m_proj = m_window->clipCorrectionMatrix();
    m_proj.perspective(m_fov, aspectRatio, 0.01f, 100.0f);
}

void Camera::onCameraViewUpdate(QPoint m_delta) // TODO: On mousewheel scroll, update m_radius, and on middle click update origin
{
    static int correction = -1;
    m_yaw += (m_delta.x() * correction) * m_sensitivity;
    m_pitch += (m_delta.y() * -1) * m_sensitivity;

    QQuaternion yawQuat = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), m_yaw);
    QQuaternion pitchQuat = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), m_pitch);

    m_rotation = yawQuat * pitchQuat;

    QVector3D offset(0, 0, m_radius);
    m_cameraPos = m_rotation.rotatedVector(offset);

    QVector3D cameraUp = m_rotation * QVector3D(0, 1, 0);

    m_updatedMatrix.setToIdentity();
    m_updatedMatrix.lookAt(m_cameraPos, QVector3D(0, 0, 0), cameraUp);

    if(cameraUp.y() < 0)
        correction = 1;
    else
        correction = -1;
}

QMatrix4x4 Camera::getProj()
{
    return m_proj * m_updatedMatrix;
}

QVector3D Camera::getPos()
{
    return m_cameraPos;
}
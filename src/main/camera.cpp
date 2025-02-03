#include "camera.h"
#include "vulkanwindow.h" 

Camera::Camera(VulkanWindow *w) : m_window(w)
{
    m_fov = 45.0f;
    m_sensitivity = 0.4f;
    m_radius = 5.0f;  
    m_yaw = 0.0f;
    m_pitch = 0.0f;

    QVector3D offset(0, 0, m_radius);
    m_cameraPos = m_rotation.rotatedVector(offset);
    m_updatedMatrix.lookAt(m_cameraPos, QVector3D(0, 0, 0), QVector3D(0, 1, 0));
    
    // updateProjection();

    // TODO: If swapchain updates, m_proj should also update, but rn it only constructs once when camera is created

    // Add swapchain init signal and onSwapchainInit slot to update m_proj
    // And make a separate matrix that will store data about current transformation so the camera view doesn't get rewritten by swapchain m_proj
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


void Camera::onCameraAxisUpdate()
{
}

void Camera::onCameraViewUpdate(QPoint m_delta)
{
    m_yaw += (m_delta.x() * -1) * m_sensitivity;
    m_pitch += (m_delta.y() * -1) * m_sensitivity;

    m_pitch = qBound(-89.9f, m_pitch, 89.9f);

    QQuaternion yawQuat = QQuaternion::fromAxisAndAngle(QVector3D(0, 1, 0), m_yaw);
    QQuaternion pitchQuat = QQuaternion::fromAxisAndAngle(QVector3D(1, 0, 0), m_pitch);

    m_rotation = yawQuat * pitchQuat;

    QVector3D offset(0, 0, m_radius);
    m_cameraPos = m_rotation.rotatedVector(offset);

    QVector3D cameraUp(0, 1, 0);

    // if((-m_pitch < 90 && -m_pitch > -90) || (-m_pitch < -270 && -m_pitch > 270))
    // {
    //     cameraUp = QVector3D(0, 1, 0); 
    //     qDebug() << "Positive: " << -m_pitch;
    // }
    // else
    // {
    //     cameraUp = QVector3D(0, -1, 0); 
    //     qDebug() << "Negative: " << -m_pitch;;
    // }
    // m_pitch = fmod(m_pitch, 360);

    m_updatedMatrix.setToIdentity();
    m_updatedMatrix.lookAt(m_cameraPos, QVector3D(0, 0, 0), cameraUp);
}

QMatrix4x4 Camera::getProj()
{
    return m_proj * m_updatedMatrix;
}
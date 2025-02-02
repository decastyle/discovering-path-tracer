#include "camera.h"
#include "vulkanwindow.h" 

Camera::Camera(VulkanWindow *w) : m_window(w)
{
    m_fov = 45.0f;
    m_sensitivity = 0.4f;
    m_cameraSpawnPos = QVector3D(0.0f, 0.0f, -5.0f);

    m_rotation.setToIdentity();
    
    updateProjection();

    // TODO: If swapchain updates, m_proj should also update, but rn it only constructs once when camera is created

    // Add swapchain init signal and onSwapchainInit slot to update m_proj
    // And make a separate matrix that will store data about current transformation so the camera view doesn't get rewritten by swapchain m_proj
}

void Camera::updateProjection() {
    const QSize sz = m_window->swapChainImageSize();
    float aspectRatio = sz.width() / static_cast<float>(sz.height());

    m_proj.setToIdentity();
    m_proj = m_window->clipCorrectionMatrix();
    m_proj.perspective(m_fov, aspectRatio, 0.01f, 100.0f);
    m_proj.translate(m_cameraSpawnPos);
}


void Camera::onCameraAxisUpdate()
{
    // m_cameraPos = m_rotation.map(m_cameraSpawnPos); 

    // qDebug() << "m_cameraPos: " << m_cameraPos;

    // m_originForward = (m_cameraPos - m_originPos).normalized();

    // m_originRight = QVector3D::crossProduct(m_originForward, m_originUp).normalized();
    // m_originRight = QVector3D(m_originRight.x(), 0, m_originRight.z()).normalized();

    // // m_originUp = QVector3D::crossProduct(m_originRight, m_originForward).normalized();
    // m_originUp = QVector3D(0, 1, 0);

    // qDebug() << "m_originForward: " << m_originForward;
    // qDebug() << "m_originRight: " << m_originRight;
    // qDebug() << "m_originUp: " << m_originUp;
}

void Camera::onCameraViewUpdate(QPoint m_delta)
{
    m_cameraPos = m_rotation.map(m_cameraSpawnPos); 

    qDebug() << "m_cameraPos: " << m_cameraPos;

    m_originForward = (m_cameraPos - m_originPos).normalized();

    m_originRight = QVector3D::crossProduct(m_originForward, m_originUp).normalized();
    m_originRight = QVector3D(m_originRight.x(), 0, m_originRight.z()).normalized();

    // m_originUp = QVector3D::crossProduct(m_originRight, m_originForward).normalized();
    m_originUp = QVector3D(0, 1, 0);

    qDebug() << "m_originForward: " << m_originForward;
    qDebug() << "m_originRight: " << m_originRight;
    qDebug() << "m_originUp: " << m_originUp;

    float angleY = m_delta.x() * m_sensitivity;
    float angleX = m_delta.y() * m_sensitivity;
 
    m_rotation.rotate(angleY, m_originUp);
    m_rotation.rotate(angleX, m_originRight.x(), 0, -m_originRight.z());

    m_updatedMatrix = m_proj * m_rotation;
}

QMatrix4x4 Camera::getProj()
{
    return m_updatedMatrix;
}
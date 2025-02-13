#include "vulkanwindow.h"
#include "vulkanrenderer.h"
#include <QWheelEvent>
#include <QMouseEvent>
#include <QCursor>
#include <QObject>
#include "camera.h"

VulkanWindow::VulkanWindow()
{
    QWindow::setCursor(Qt::OpenHandCursor);
    m_camera = new Camera(this);
    
    QObject::connect(this, &VulkanWindow::cameraViewUpdate, m_camera, &Camera::onCameraViewUpdate);
    QObject::connect(this, &VulkanWindow::cameraZoomUpdate, m_camera, &Camera::onCameraZoomUpdate);
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    m_renderer = new VulkanRenderer(this);
    m_raytracer = new VulkanRayTracer(this);

    QObject::connect(m_renderer->m_helper, &VulkanRendererHelper::updateSwapChain, m_camera, &Camera::onUpdateSwapChain);
    QObject::connect(m_renderer->m_helper, &VulkanRendererHelper::deviceReady, m_raytracer, &VulkanRayTracer::onDeviceReady);

    return m_renderer;
}

VulkanRayTracer *VulkanWindow::getVulkanRayTracer()
{
    return m_raytracer;
}

Camera *VulkanWindow::getCamera()
{
    return m_camera;
}

void VulkanWindow::wheelEvent(QWheelEvent *event)
{
    qDebug() << event->angleDelta().y();
    
    m_zoom = 1.0;
    
    if (event->angleDelta().y() > 0)
    {
        m_zoom = 0.9;
    }
    else
    {
        m_zoom = 1.1;
    }

    emit cameraZoomUpdate(m_zoom);
}



void VulkanWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {
        m_lastCursorPos = event->pos();  
        QWindow::setCursor(Qt::ClosedHandCursor);
    }
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event)
{
    static bool justWarped = false;

    if (justWarped) {
        justWarped = false;
        return;
    }

    QPoint globalPos = mapToGlobal(event->pos());
    QRect windowRect = this->geometry();
    QPoint topLeft = mapToGlobal(QPoint(0, 0));
    int width = windowRect.width();
    int height = windowRect.height();

    QPoint newPos = globalPos;  // Default, no change

    // Check if we need to wrap
    if (globalPos.x() >= topLeft.x() + width) {
        newPos.setX(topLeft.x());  // Wrap to left
        justWarped = true;
    } else if (globalPos.x() < topLeft.x()) {
        newPos.setX(topLeft.x() + width - 1);  // Wrap to right
        justWarped = true;
    }
    if (globalPos.y() >= topLeft.y() + height) {
        newPos.setY(topLeft.y());  // Wrap to top
        justWarped = true;
    } else if (globalPos.y() < topLeft.y()) {
        newPos.setY(topLeft.y() + height - 1);  // Wrap to bottom
        justWarped = true;
    }

    if (justWarped) {
        QCursor::setPos(newPos);
        m_lastCursorPos = mapFromGlobal(newPos);  // **Update only after warp**
        return;
    }

    if (event->buttons() & Qt::LeftButton) 
    {
        m_delta = event->pos() - m_lastCursorPos;
        m_lastCursorPos = event->pos();
        emit cameraViewUpdate(m_delta);
    }
}

void VulkanWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {  
        QWindow::setCursor(Qt::OpenHandCursor);
    }
}
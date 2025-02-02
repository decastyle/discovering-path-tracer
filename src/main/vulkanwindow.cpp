#include "vulkanwindow.h"
#include "vulkanrenderer.h"
#include <QMouseEvent>
#include <QCursor>
#include <QObject>
#include "camera.h"

VulkanWindow::VulkanWindow()
{
    QWindow::setCursor(Qt::OpenHandCursor);
    m_camera = new Camera(this);
    QObject::connect(this, &VulkanWindow::cameraViewUpdate, m_camera, &Camera::onCameraViewUpdate);
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    m_renderer = new VulkanRenderer(this);

    QObject::connect(m_renderer->m_helper, &VulkanRendererHelper::updateSwapChain, m_camera, &Camera::onUpdateSwapChain);

    return m_renderer;
    
}

Camera *VulkanWindow::getCamera()
{
    return m_camera;
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

/* 
QVulkanWindowRenderer *QVulkanWindow::createRenderer()

Returns a new instance of QVulkanWindowRenderer.

This virtual function is called once during the lifetime of the window, at some point after making it visible for the first time.

The default implementation returns null and so no rendering will be performed apart from clearing the buffers.

The window takes ownership of the returned renderer object.
*/

// i have no idea to what member it returns the pointer to renderer
// nvm i don't need to know lol
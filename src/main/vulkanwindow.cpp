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

/* 
QVulkanWindowRenderer *QVulkanWindow::createRenderer()

Returns a new instance of QVulkanWindowRenderer.

This virtual function is called once during the lifetime of the window, at some point after making it visible for the first time.

The default implementation returns null and so no rendering will be performed apart from clearing the buffers.

The window takes ownership of the returned renderer object.
*/

// i have no idea to what member it returns the pointer to renderer
// nvm i don't need to know lol
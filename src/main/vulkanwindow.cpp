#include "vulkanwindow.h"
#include "vulkanrenderer.h"
#include <QMouseEvent>
#include <QCursor>

VulkanWindow::VulkanWindow()
{
    QWindow::setCursor(Qt::OpenHandCursor);
}

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    return new VulkanRenderer(this);
    
}


void VulkanWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) 
    {
        m_lastCursorPos = event->pos();  
        qDebug() << m_lastCursorPos;
        QWindow::setCursor(Qt::ClosedHandCursor);
    }
}

void VulkanWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) 
    {  
        m_delta += event->pos() - m_lastCursorPos;
        m_lastCursorPos = event->pos(); 
        m_delta.setX(m_delta.x() % 360);
        m_delta.setY(m_delta.y() % 360);
        qDebug() << m_delta;
        
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
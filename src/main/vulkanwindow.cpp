#include "vulkanwindow.h"
#include "vulkanrenderer.h"

QVulkanWindowRenderer *VulkanWindow::createRenderer()
{
    return new VulkanRenderer(this);
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
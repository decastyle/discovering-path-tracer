#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindow>

class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(QVulkanWindow *w);
    void initResources(); // The default implementation is empty.
    void startNextFrame() override;

protected:
    QVulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    float m_hue = 0;
    
};

#endif // VULKANRENDERER_H
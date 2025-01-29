#ifndef VULKANRENDERER_H
#define VULKANRENDERER_H

#include <QVulkanWindow>

class VulkanRenderer : public QVulkanWindowRenderer
{
public:
    VulkanRenderer(QVulkanWindow *w);
    void initResources() override;

    // void initSwapChainResources() override;
    // void releaseSwapChainResources() override;
    
    // void releaseResources() override;

    void startNextFrame() override;

protected:
    QVulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    float m_hue = 0;
    
};

#endif // VULKANRENDERER_H
#ifndef VULKANRAYTRACER_H
#define VULKANRAYTRACER_H

#include <QObject>
#include <QString>
#include <QWindow>
#include <QCursor>
#include <QFile>
#include <QVulkanWindow>
#include <QVulkanFunctions>
#include "vulkanwindow.h"

class VulkanWindow;

class VulkanRayTracer : public QObject
{
    Q_OBJECT

public:
    VulkanRayTracer(VulkanWindow *w);

public slots:
    void onDeviceReady(); // TODO: Find a better way to initialize device (right now ray-tracer waits for renderer to initialize it through QVulkanWindow)

private:

    uint32_t findComputeQueueFamilyIndex(VkPhysicalDevice physicalDevice);
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    VkShaderModule createShaderModule(const QString &filename);
    VkQueue m_computeQueue;

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_computePipeline = VK_NULL_HANDLE;
};


#endif // VULKANRAYTRACER_H
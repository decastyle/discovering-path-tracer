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
    VulkanRayTracer();

private:
    VulkanWindow *m_window;
    QVulkanDeviceFunctions *m_devFuncs;
    VkShaderModule createShaderModule(const QString &filename);

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
};


#endif // VULKANRAYTRACER_H
#ifndef VULKANDOW_H
#define VULKANWINDOW_H

#include <QObject>
#include <QVulkanWindow>

class VulkanWindow : public QVulkanWindow
{
public:
    QVulkanWindowRenderer *createRenderer() override;
};

#endif // VULKANWINDOW_H
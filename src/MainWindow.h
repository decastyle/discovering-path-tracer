#pragma once

#include <QWidget>

#include "VulkanWindow.h"

class MainWindow : public QWidget 
{
public:
    MainWindow(VulkanWindow* vulkanWindow);

private:
    VulkanWindow* m_vulkanWindow = nullptr;
};
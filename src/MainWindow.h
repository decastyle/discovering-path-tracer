#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLCDNumber>
#include <QCursor>
#include <QVulkanFunctions>
#include <QApplication>
#include <QVBoxLayout>

#include "VulkanRenderer.h"
#include "VulkanWindow.h"

class MainWindow : public QWidget
{
public:
    MainWindow(VulkanWindow *VulkanWindow);

private:
    VulkanWindow *m_vulkanWindow;
    // QTabWidget *m_infoTab;
    // QPlainTextEdit *m_info;
    // QLCDNumber *m_number;
};

// TODO: Make GUI look good
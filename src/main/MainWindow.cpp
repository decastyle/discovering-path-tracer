#include <QVulkanFunctions>
#include <QApplication>
#include <QVBoxLayout>

#include "MainWindow.h"
#include "VulkanWindow.h"

MainWindow::MainWindow(VulkanWindow *vulkanWindow)
    : m_vulkanWindow(vulkanWindow)
{
    QWidget *wrapper = QWidget::createWindowContainer(vulkanWindow);

    // m_info = new QPlainTextEdit;
    // m_info->setReadOnly(true);

    // m_number = new QLCDNumber(3);
    // m_number->setSegmentStyle(QLCDNumber::Filled);

    // QPushButton *quitButton = new QPushButton(tr("&Quit")   );
    // quitButton->setFocusPolicy(Qt::NoFocus);

    // connect(quitButton, &QPushButton::clicked, qApp, &QCoreApplication::quit); // only valid use of connect lol

    QVBoxLayout *layout = new QVBoxLayout;
    // m_infoTab = new QTabWidget(this);
    // m_infoTab->addTab(m_info, tr("Vulkan Info"));
    // m_infoTab->addTab(logWidget, tr("Debug Log"));
    // layout->addWidget(m_infoTab, 2);
    // layout->addWidget(m_number, 1);
    // layout->addWidget(quitButton, 1);
    layout->addWidget(wrapper);
    setLayout(layout);
}


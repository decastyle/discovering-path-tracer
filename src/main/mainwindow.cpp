#include <QVulkanFunctions>
#include <QApplication>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLCDNumber>
#include <QFileDialog>
#include <QMessageBox>
#include <QTabWidget>
#include "mainwindow.h"
#include "vulkanwindow.h"
#include <QElapsedTimer>

MainWindow::MainWindow(VulkanWindow *w)
    : m_window(w)
{
    QWidget *wrapper = QWidget::createWindowContainer(w);

    // m_info = new QPlainTextEdit;
    // m_info->setReadOnly(true);

    // m_number = new QLCDNumber(3);
    // m_number->setSegmentStyle(QLCDNumber::Filled);

    // QPushButton *quitButton = new QPushButton(tr("&Quit")   );
    // quitButton->setFocusPolicy(Qt::NoFocus);

    // connect(quitButton, &QPushButton::clicked, qApp, &QCoreApplication::quit);

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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "vulkanrenderer.h"
#include <QWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLCDNumber>
#include <QCursor>
#include "vulkanwindow.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(VulkanWindow *w);

public slots:
    void onVulkanInfoReceived(const QString &text);
    void onFrameQueued();

private:

    VulkanWindow *m_window;
    QTabWidget *m_infoTab;
    QPlainTextEdit *m_info;
    QLCDNumber *m_number;

    QElapsedTimer timer;
    int frameCount;
};

#endif // MAINWINDOW_H

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "VulkanRenderer.h"
#include <QWidget>
#include <QTabWidget>
#include <QPlainTextEdit>
#include <QLCDNumber>
#include <QCursor>
#include "VulkanWindow.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(VulkanWindow *w);

private:
    VulkanWindow *m_window;
    // QTabWidget *m_infoTab;
    // QPlainTextEdit *m_info;
    // QLCDNumber *m_number;
};

#endif // MAINWINDOW_H

// TODO: Make GUI look good
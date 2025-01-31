#ifndef VULKANWINDOW_H
#define VULKANWINDOW_H

#include <QObject>
#include <QVulkanWindow>
#include <QString>
#include <QWindow>
#include <QCursor>

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    VulkanWindow();
    QVulkanWindowRenderer *createRenderer() override;
    QPoint m_delta{};

signals:
    void vulkanInfoReceived(const QString &text);
    void frameQueued();
    
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPoint m_lastCursorPos;
};

#endif // VULKANWINDOW_H
#include <QApplication>
#include <QWidget>
#include <QVulkanWindow>
#include <vulkan/vulkan.h>
#include <QThread>
#include <QLoggingCategory>
#include "worker.h"
static const bool DEBUG = false;

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QVulkanInstance instance;

    if(DEBUG) {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
        instance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    }

    if (!instance.create()) {
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());
    }

    QVulkanWindow window;
    window.setVulkanInstance(&instance);

    window.resize(1024, 768);
    window.show();

    // Worker and thread setup
    QThread *thread = new QThread;
    Worker *worker = new Worker;

    worker->moveToThread(thread);

    QObject::connect(thread, &QThread::started, worker, &Worker::doHeavyTask, Qt::QueuedConnection);
    QObject::connect(worker, &Worker::taskFinished, thread, &QThread::quit, Qt::QueuedConnection);
    QObject::connect(worker, &Worker::taskFinished, worker, &Worker::deleteLater, Qt::QueuedConnection);
    QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater, Qt::QueuedConnection);

    thread->start();

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() {
        if (thread->isRunning()) {
            thread->quit();
            thread->wait();  
        }
    });

    return app.exec();
}

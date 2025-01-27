#include <QApplication>
#include <QWidget>
#include <QVulkanWindow>
#include <vulkan/vulkan.h>
#include <QThread>
#include <QLoggingCategory>
#include "worker.h"
#include "vulkanwindow.h"
#include "vulkanrenderer.h"

static const bool DEBUG = false;

int main(int argc, char *argv[]) 
{
    QApplication app(argc, argv);

    QVulkanInstance instance;

    if(DEBUG) 
    {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
        instance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    }

    if (!instance.create()) 
    {
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());
    }

    VulkanWindow window;
    /*
    QVulkanWindow is a Vulkan-capable QWindow that manages a Vulkan device, a graphics queue, a command pool and buffer, a depth-stencil image and a double-buffered FIFO swapchain, while taking care of correct behavior when it comes to events like resize, special situations like not having a device queue supporting both graphics and presentation, device lost scenarios, and additional functionality like reading the rendered content back. Conceptually it is the counterpart of QOpenGLWindow in the Vulkan world.
    */

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

    QObject::connect(&app, &QApplication::aboutToQuit, [&]() 
    {
        if (thread->isRunning()) 
        {
            thread->quit();
            thread->wait();  
        }
    });

    /*
    We recommend that you connect clean-up code to the aboutToQuit() signal, instead of putting it in your application's main() function. This is because, on some platforms the QApplication::exec() call may not return. For example, on the Windows platform, when the user logs off, the system terminates the process after Qt closes all top-level windows. Hence, there is no guarantee that the application will have time to exit its event loop and execute code at the end of the main() function, after the QApplication::exec() call.
    */
    return app.exec();
}

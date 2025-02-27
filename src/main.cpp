#include <QApplication>
#include <QLoggingCategory>
#include <vulkan/vulkan.h>

#include "MainWindow.h"
#include "VulkanWindow.h"

static const bool DEBUG = true;

int main(int argc, char *argv[]) 
{
    QApplication app(argc, argv);

    QVulkanInstance instance;

    instance.setApiVersion(QVersionNumber(1, 4));

    // instance.setExtensions({
    //     VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, 
    //     VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,       
    //     VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,         
    //     VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,             
    //     VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,          
    //     VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,  
    //     VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
    // });

    if (DEBUG) 
    {
        QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
        instance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    }

    if (!instance.create()) 
    {
        qWarning("Failed to create Vulkan instance (error code: %d)", instance.errorCode());
    }

    VulkanWindow* vulkanWindow = new VulkanWindow;

    /*
    QVulkanWindow is a Vulkan-capable QWindow that manages a Vulkan device, a graphics queue, a command pool and buffer, a depth-stencil image and a double-buffered FIFO swapchain, while taking care of correct behavior when it comes to events like resize, special situations like not having a device queue supporting both graphics and presentation, device lost scenarios, and additional functionality like reading the rendered content back. Conceptually it is the counterpart of QOpenGLWindow in the Vulkan world.
    */

    vulkanWindow->setFlags(QVulkanWindow::PersistentResources);
    /*
    Ensures no graphics resources are released when the window becomes unexposed. The default behavior is to release everything, and reinitialize later when becoming visible again.
    */
    
    vulkanWindow->setVulkanInstance(&instance);

    MainWindow mainWindow(vulkanWindow);

    mainWindow.resize(1024, 1024);
    mainWindow.show();

    /*
    We recommend that you connect clean-up code to the aboutToQuit() signal, instead of putting it in your application's main() function. This is because, on some platforms the QApplication::exec() call may not return. For example, on the Windows platform, when the user logs off, the system terminates the process after Qt closes all top-level windows. Hence, there is no guarantee that the application will have time to exit its event loop and execute code at the end of the main() function, after the QApplication::exec() call.
    */

    return app.exec();
}

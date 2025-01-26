#include <QApplication>
#include <QWidget>
#include <vulkan/vulkan.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    const char* appName = "discovering-path-tracer";

    QWidget window;
    window.setWindowTitle(appName);
    window.resize(800, 600);
    window.show();

    VkInstance instance;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        qFatal("Failed to create Vulkan instance!");
    }

    return app.exec();
}

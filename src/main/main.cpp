#include <QApplication>
#include <QWidget>
#include <QVulkanWindow>
#include <vulkan/vulkan.h>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QVulkanInstance instance;
    instance.setLayers({ "VK_LAYER_KHRONOS_validation" });
    if (!instance.create())
        qFatal("Failed to create Vulkan instance: %d", instance.errorCode());

    QVulkanWindow window;
    window.setVulkanInstance(&instance);

    window.resize(1024, 768);
    window.show();

    return app.exec();
}

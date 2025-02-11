#include "vulkanraytracer.h"

VkShaderModule VulkanRayTracer::createShaderModule(const QString& filename)
{
    VkShaderModule shaderModule;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Failed to read shader %s", qPrintable(filename));
        shaderModule = VK_NULL_HANDLE;
    }
    QByteArray blob = file.readAll();
    file.close();

    VkShaderModuleCreateInfo shaderInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = static_cast<size_t>(blob.size()),
        .pCode = reinterpret_cast<const uint32_t *>(blob.constData())
    };


    VkResult result = m_devFuncs->vkCreateShaderModule(m_window->device(), &shaderInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) 
    {
        qWarning("Failed to create shader module: %d", result);
        shaderModule = VK_NULL_HANDLE;
    }
    else
    {
        qDebug("Shaders loaded successfully!");
    }

    return shaderModule;
}

VulkanRayTracer::VulkanRayTracer()
{
    // TODO: Initialize compute pipeline

    QVulkanInstance *inst = m_window->vulkanInstance();
    VkDevice dev = m_window->device();
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);

    VkResult result;

    VkShaderModule rayTraceModule = createShaderModule(QStringLiteral(":/raytrace_comp.spv"));

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo
    {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = rayTraceModule,
        .pName  = "main"
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 0,
        .pSetLayouts = VK_NULL_HANDLE,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    
    result = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", result);

    VkComputePipelineCreateInfo pipelineCreateInfo
    {
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,  //
        .stage  = shaderStageCreateInfo,                           //
        .layout = m_pipelineLayout
    };

    VkPipeline computePipeline;

    result = m_devFuncs->vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &pipelineCreateInfo, VK_NULL_HANDLE, &computePipeline);
    if (result != VK_SUCCESS)
        qFatal("Failed to create compute pipeline: %d", result);
}

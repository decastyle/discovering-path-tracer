#include "VulkanRenderer.h"
#include "VulkanWindow.h"
#include <QVulkanFunctions>
#include <thread>

static const uint64_t render_width     = 1024; // TODO: Pass this data dynamically through Qt's GUI
static const uint64_t render_height    = 1024;
static const uint32_t workgroup_width  = 16;
static const uint32_t workgroup_height = 16;

// TODO: Include .obj loader and a button "Load .obj" to load vertexData (update vertex input locations)

// static float vertexData[] = {
//     // Position (x, y, z)  // Normal (nx, ny, nz) // UV (u, v)
//
//     // Front face (Z = 1.0f)
//     -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
//      0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,
//      0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,
//
//      0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,
//     -0.5f,  0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f,  0.5f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
//
//     // Back face (Z = -1.0f)
//     -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,
//      0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,
//      0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 0.0f,
//
//      0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 0.0f,
//     -0.5f,  0.5f, -0.5f,    0.0f,  0.0f, -1.0f,    1.0f, 1.0f,
//
//     // Left face (X = -1.0f)
//     -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//     -0.5f, -0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
//     -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//
//     -0.5f,  0.5f,  0.5f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//     -0.5f,  0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f, -0.5f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//
//     // Right face (X = 1.0f)
//      0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//      0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//      0.5f, -0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f,
//
//      0.5f,  0.5f,  0.5f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f,
//      0.5f, -0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f,
//      0.5f,  0.5f, -0.5f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f,
//
//     // Top face (Y = 1.0f)
//     -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,
//      0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
//      0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f,
//
//      0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f,
//     -0.5f,  0.5f, -0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f,
//     -0.5f,  0.5f,  0.5f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f,
//
//     // Bottom face (Y = -1.0f)
//     -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
//      0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f,
//      0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
//
//      0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f,
//     -0.5f, -0.5f,  0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f,
//     -0.5f, -0.5f, -0.5f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f,
// };

static float vertexData[] = {
    // Front face (Z = 1.0f)
    -0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
     0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f, 1.0f,
     0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,

     0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    1.0f, 0.0f,
    -0.5f,  0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 0.0f,
    -0.5f, -0.5f,  0.0f,    0.0f,  0.0f,  1.0f,    0.0f, 1.0f,
};

static const int UNIFORM_MATRIX_DATA_SIZE = 16 * sizeof(float);
static const int UNIFORM_VECTOR_DATA_SIZE = 3 * sizeof(float);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VulkanRenderer::VulkanRenderer(VulkanWindow *vulkanWindow)
    : m_vulkanWindow(vulkanWindow)
{   
    const QList<int> counts = vulkanWindow->supportedSampleCounts();

    qDebug() << "Supported sample counts:" << counts;
    for (int s = 16; s >= 4; s /= 2) 
    {
        if (counts.contains(s)) 
        {
            qDebug("Requesting sample count %d", s);
            m_vulkanWindow->setSampleCount(s);
            break;
        }
    }
}

void VulkanRenderer::initResources()
{
    m_device = m_vulkanWindow->device();
    m_deviceFunctions = m_vulkanWindow->vulkanInstance()->deviceFunctions(m_device);

    m_graphicsQueueFamilyIndex = m_vulkanWindow->findQueueFamilyIndex(m_vulkanWindow->physicalDevice(), VK_QUEUE_GRAPHICS_BIT);
    if (m_graphicsQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable graphics queue family found!");
        
    m_computeQueueFamilyIndex = m_vulkanWindow->findQueueFamilyIndex(m_vulkanWindow->physicalDevice(), VK_QUEUE_COMPUTE_BIT);
    if (m_computeQueueFamilyIndex == UINT32_MAX)
        qDebug("No suitable compute queue family found!");

    m_deviceFunctions->vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
    m_graphicsCommandPool = VulkanCommandPool(m_vulkanWindow, m_graphicsQueueFamilyIndex);

    const int concurrentFrameCount = m_vulkanWindow->concurrentFrameCount(); 
    qDebug() << "Concurrent frame count:" << concurrentFrameCount;
    
    const VkPhysicalDeviceLimits *pdevLimits = &m_vulkanWindow->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    // const float maxSamplerAnisotropy = pdevLimits->maxSamplerAnisotropy;
    qDebug("Uniform buffer offset alignment is %u", (uint) uniAlign);

    /////////////////////////////////////////////////////////////////////
    // Create buffers
    /////////////////////////////////////////////////////////////////////

    m_vertexBuffer = VulkanBuffer(m_vulkanWindow, 
                                sizeof(vertexData), 
                                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                                m_vulkanWindow->deviceLocalMemoryIndex());

    m_vertexStagingBuffer = VulkanBuffer(m_vulkanWindow, 
                                        sizeof(vertexData), 
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
                                        m_vulkanWindow->hostVisibleMemoryIndex());
    m_vertexStagingBuffer.copyData(vertexData, sizeof(vertexData));

    const VkDeviceSize uniformBufferDeviceSize = aligned(UNIFORM_MATRIX_DATA_SIZE, uniAlign) + aligned(UNIFORM_VECTOR_DATA_SIZE, uniAlign);
    m_uniformBuffer = VulkanBuffer(m_vulkanWindow, 
                                uniformBufferDeviceSize * concurrentFrameCount, 
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                m_vulkanWindow->hostVisibleMemoryIndex());
                    
    QMatrix4x4 identityMatrix;
    for (int i = 0; i < concurrentFrameCount; ++i) 
    {
        const VkDeviceSize offset = i * uniformBufferDeviceSize;
        m_uniformBuffer.copyData(identityMatrix.constData(), 16 * sizeof(float), offset);

        m_uniformBufferInfo[i] = VkDescriptorBufferInfo{
            .buffer = m_uniformBuffer.getBuffer(),
            .offset = offset,
            .range = uniformBufferDeviceSize
        };
    }

    /////////////////////////////////////////////////////////////////////
    // Create image
    /////////////////////////////////////////////////////////////////////

    m_renderImage = VulkanImage(m_vulkanWindow, 
                                render_width, render_height, 
                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 
                                m_vulkanWindow->deviceLocalMemoryIndex());

    {
        VulkanCommandBuffer commandBuffer = VulkanCommandBuffer(m_vulkanWindow, m_graphicsCommandPool.getCommandPool(), m_graphicsQueue);

        commandBuffer.beginSingleTimeCommandBuffer();

        VkImageMemoryBarrier imageMemoryBarrierToShaderRead
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = m_renderImage.getImage(),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };  

        m_deviceFunctions->vkCmdPipelineBarrier(
        commandBuffer.getCommandBuffer(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0, nullptr,
        0, nullptr, 
        1, &imageMemoryBarrierToShaderRead);   

        // VkImageCopy bufferCopyRegion
        // {
        //     .srcSubresource = {
        //         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        //         .mipLevel = 0,
        //         .baseArrayLayer = 0,
        //         .layerCount = 1
        //     },
        //     .srcOffset = {0, 0, 0},
        //     .dstSubresource = {
        //         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        //         .mipLevel = 0,
        //         .baseArrayLayer = 0,
        //         .layerCount = 1
        //     },
        //     .dstOffset = {0, 0, 0},
        //     .extent = {
        //         .width = render_width,
        //         .height = render_height,
        //         .depth = 1
        //     }
        // };

        // VkImage storageImage = m_vulkanWindow->getVulkanRayTracer()->getStorageImage(); 

        // m_deviceFunctions->vkCmdCopyImage(
        //     commandBuffer.getCommandBuffer(), 
        //     storageImage, 
        //     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        //     m_renderImage.getImage(), 
        //     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        //     1, &bufferCopyRegion);

        // VkImageMemoryBarrier imageMemoryBarrierToShaderRead
        // {
        //     .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        //     .pNext = nullptr,
        //     .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        //     .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        //     .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //     .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        //     .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        //     .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        //     .image = m_renderImage.getImage(),
        //     .subresourceRange = {
        //         .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        //         .baseMipLevel = 0,
        //         .levelCount = 1,
        //         .baseArrayLayer = 0,
        //         .layerCount = 1
        //     }
        // };  

        // m_deviceFunctions->vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(),
        // VK_PIPELINE_STAGE_TRANSFER_BIT,
        // VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        // 0,
        // 0, nullptr,
        // 0, nullptr, 
        // 1, &imageMemoryBarrierToShaderRead);   

        commandBuffer.endSubmitAndWait();
    }

    /////////////////////////////////////////////////////////////////////
    // Pipeline shader stages
    /* More info...
    It uses .spv aliases defined in shaders.qrc
    All shaders are compiled automatically during CMake configuration with glslangValidator.exe
    */
    /////////////////////////////////////////////////////////////////////

    VkShaderModule vertexShaderModule = m_vulkanWindow->createShaderModule(QStringLiteral(":/color_vert.spv"));
    VkShaderModule fragmentShaderModule = m_vulkanWindow->createShaderModule(QStringLiteral(":/color_frag.spv"));

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShaderModule,
            .pName = "main",
            .pSpecializationInfo = nullptr
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShaderModule,
            .pName = "main",
            .pSpecializationInfo = nullptr
        }
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline vertex input state
    /* More info...
    The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data that will be passed to the vertex shader. It describes this in roughly two ways:

    Bindings: spacing between data and whether the data is per-vertex or per-instance (see instancing)
    Attribute descriptions: type of the attributes passed to the vertex shader, which binding to load them from and at which offset
    */
    /////////////////////////////////////////////////////////////////////

    // TODO: Update vertex input binding description, it should include vertices, normals and UV coordinates

    VkVertexInputBindingDescription vertexInputBindingDescription = {
        .binding = 0,
        .stride = 8 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertexInputAttributeDescription[] = {
        { // position
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        },
        { // normal
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 3 * sizeof(float)
        },
        { // uv
            .location = 2,
            .binding = 0,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = 6 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = 3,
        .pVertexAttributeDescriptions = vertexInputAttributeDescription
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline input assembly state
    /* More info...
    The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the vertices and if primitive restart should be enabled. The former is specified in the topology member and can have values like:

        VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
        VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every 2 vertices without reuse
        VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every 3 vertices without reuse
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle
    Normally, the vertices are loaded from the vertex buffer by index in sequential order, but with an element buffer you can specify the indices to use yourself. This allows you to perform optimizations like reusing vertices. If you set the primitiveRestartEnable member to VK_TRUE, then it's possible to break up lines and triangles in the _STRIP topology modes by using a special index of 0xFFFF or 0xFFFFFFFF.
    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline viewport state
    /* More info...
    
    Viewport(s) and scissor rectangle(s) can either be specified as a static part of the pipeline or as a dynamic state set in the command buffer. While the former is more in line with the other states it's often convenient to make viewport and scissor state dynamic as it gives you a lot more flexibility. This is very common and all implementations can handle this dynamic state without a performance penalty.

    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .viewportCount = 1,
        .pViewports = nullptr,  // Optional: set to nullptr if not using custom viewports
        .scissorCount = 1,
        .pScissors = nullptr    // Optional: set to nullptr if not using custom scissors
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline rasterization state
    /* More info...
    The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader. It also performs depth testing, face culling and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering). All this is configured using the VkPipelineRasterizationStateCreateInfo structure.
    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f, // Optional
        .depthBiasClamp = 0.0f,         // Optional
        .depthBiasSlopeFactor = 0.0f,    // Optional
        .lineWidth = 1.0f
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline multisample state
    /* More info...
    The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the ways to perform anti-aliasing. It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel. This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur. Because it doesn't need to run the fragment shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than simply rendering to a higher resolution and then downscaling. Enabling it requires enabling a GPU feature.
    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = m_vulkanWindow->sampleCountFlagBits(),
        .sampleShadingEnable = VK_TRUE,
        .minSampleShading = 1.0f,           // Optional
        .pSampleMask = nullptr,            // Optional
        .alphaToCoverageEnable = VK_FALSE, // Optional
        .alphaToOneEnable = VK_FALSE      // Optional
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline depth stencil state
    /* More info...

    */
    /////////////////////////////////////////////////////////////////////

    VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .depthBoundsTestEnable = VK_FALSE,  // Optional: set to VK_FALSE if not using depth bounds test
        .stencilTestEnable = VK_FALSE,      // Optional: set to VK_FALSE if not using stencil test
        .front = {},                        // Optional: stencil front settings if needed
        .back = {},                         // Optional: stencil back settings if needed
        .minDepthBounds = 0.0f,             // Optional
        .maxDepthBounds = 1.0f              // Optional
    };


    /////////////////////////////////////////////////////////////////////
    // Pipeline color blend state
    /////////////////////////////////////////////////////////////////////

    VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &pipelineColorBlendAttachmentState,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // Optional
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline dynamic state
    /* More info...
    While most of the pipeline state needs to be baked into the pipeline state, a limited amount of the state can actually be changed without recreating the pipeline at draw time. Examples are the size of the viewport, line width and blend constants. 

    This will cause the configuration of these values to be ignored and you will be able (and required) to specify the data at drawing time. This results in a more flexible setup and is very common for things like viewport and scissor state, which would m_result in a more complex setup when being baked into the pipeline state.
    */
    /////////////////////////////////////////////////////////////////////

    VkDynamicState dynamicStates[] = { 
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR 
    };

    VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
        .pDynamicStates = dynamicStates
    };
    
    /////////////////////////////////////////////////////////////////////
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descriptorPoolSizes[]
    {
        {
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = uint32_t(concurrentFrameCount)   
        },
        {
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = uint32_t(concurrentFrameCount)
        }
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = static_cast<uint32_t>(concurrentFrameCount),
        .poolSizeCount = 2,
        .pPoolSizes = descriptorPoolSizes
    };

    m_result = m_deviceFunctions->vkCreateDescriptorPool(m_device, &descriptorPoolCreateInfo, nullptr, &m_descriptorPool);
    if (m_result != VK_SUCCESS)
        qDebug("Failed to create descriptor pool: %d", m_result);

    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding[] 
    {
        {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .pImmutableSamplers = nullptr
        },
        {
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr
        },
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 2,
        .pBindings = descriptorSetLayoutBinding
    };

    m_result = m_deviceFunctions->vkCreateDescriptorSetLayout(m_device, &descriptorSetLayoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create descriptor set layout (error code: %d)", m_result);
        return;
    }

    for (int i = 0; i < concurrentFrameCount; ++i) {

        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descriptorSetLayout
        };

        m_result = m_deviceFunctions->vkAllocateDescriptorSets(m_device, &descriptorSetAllocateInfo, &m_descriptorSet[i]);
        if (m_result != VK_SUCCESS)
        {
            qWarning("Failed to allocate descriptor set (error code: %d)", m_result);
            return;
        }

        VkWriteDescriptorSet uniformBufferWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = m_descriptorSet[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &m_uniformBufferInfo[i],
            .pTexelBufferView = nullptr
        };

        VkDescriptorImageInfo m_descriptorImageInfo 
        {
            .sampler = m_renderImage.getSampler(),
            .imageView = m_renderImage.getImageView(),
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet storageImageWrite
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = m_descriptorSet[i],
            .dstBinding = 1, // Binding 1 is for storage image
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &m_descriptorImageInfo,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr
        };

        VkWriteDescriptorSet descriptorWrites[] = { storageImageWrite, uniformBufferWrite };
        
        m_deviceFunctions->vkUpdateDescriptorSets(m_device, 2, descriptorWrites, 0, nullptr);
    }

    /////////////////////////////////////////////////////////////////////
    // Pipeline layout
    /////////////////////////////////////////////////////////////////////

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = nullptr
    };

    m_result = m_deviceFunctions->vkCreatePipelineCache(m_device, &pipelineCacheCreateInfo, nullptr, &m_pipelineCache);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create pipeline cache (error code: %d)", m_result);
        return;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descriptorSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    m_result = m_deviceFunctions->vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create pipeline layout (error code: %d)", m_result);
        return;
    }

    /////////////////////////////////////////////////////////////////////
    // Graphics pipeline
    /////////////////////////////////////////////////////////////////////

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &pipelineVertexInputStateCreateInfo,
        .pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo,
        .pTessellationState = nullptr,
        .pViewportState = &pipelineViewportStateCreateInfo,
        .pRasterizationState = &pipelineRasterizationStateCreateInfo,
        .pMultisampleState = &pipelineMultisampleStateCreateInfo,
        .pDepthStencilState = &pipelineDepthStencilStateCreateInfo,
        .pColorBlendState = &pipelineColorBlendStateCreateInfo,
        .pDynamicState = &pipelineDynamicStateCreateInfo,
        .layout = m_pipelineLayout,
        .renderPass = m_vulkanWindow->defaultRenderPass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    m_result = m_deviceFunctions->vkCreateGraphicsPipelines(m_device, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (m_result != VK_SUCCESS)
    {
        qWarning("Failed to create graphics pipeline (error code: %d)", m_result);
        return;
    }

    /////////////////////////////////////////////////////////////////////
    // Initialize VulkanRayTracer in separate thread
    /////////////////////////////////////////////////////////////////////

    std::thread rayTracerThread([this]() {
        m_vulkanWindow->getVulkanRayTracer()->initRayTracer();
    });

    rayTracerThread.detach();

    /////////////////////////////////////////////////////////////////////
    // Clean up
    /////////////////////////////////////////////////////////////////////

    if (vertexShaderModule)
        m_deviceFunctions->vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
    if (fragmentShaderModule)
        m_deviceFunctions->vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
}

void VulkanRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");

    m_vulkanWindow->getCamera()->cameraSwapChainUpdate();
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");

    if (m_pipeline) 
    {
        m_deviceFunctions->vkDestroyPipeline(m_device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout) 
    {
        m_deviceFunctions->vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineCache) 
    {
        m_deviceFunctions->vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout) 
    {
        m_deviceFunctions->vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorPool) 
    {
        m_deviceFunctions->vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    // m_graphicsCommandPool.destroy();
    // m_vertexBuffer.destroy();
    // m_vertexStagingBuffer.destroy();
    // m_uniformBuffer.destroy();
    // m_renderImage.destroy();
}

void VulkanRenderer::startNextFrame()
{
    m_renderTimer.start();

    QSize swapChainSize = m_vulkanWindow->swapChainImageSize();
    VkCommandBuffer commandBuffer = m_vulkanWindow->currentCommandBuffer();
    uint32_t currentFrame = m_vulkanWindow->currentFrame();

    /////////////////////////////////////////////////////////////////////
    // Copy staging buffer to vertex buffer
    /////////////////////////////////////////////////////////////////////

    VkBufferCopy bufferCopyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sizeof(vertexData)
    };

    m_deviceFunctions->vkCmdCopyBuffer(commandBuffer, 
                                    m_vertexStagingBuffer.getBuffer(), 
                                    m_vertexBuffer.getBuffer(), 
                                    1, &bufferCopyRegion);

    /////////////////////////////////////////////////////////////////////
    // Pipeline barrier to ensure staging buffer copy completes before rendering
    /////////////////////////////////////////////////////////////////////

    // TODO: Do not copy if the data is the same

    VkMemoryBarrier memoryBarrier = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT
    };

    m_deviceFunctions->vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,         
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,    
        0,
        1, &memoryBarrier,  // Global memory barrier
        0, nullptr,
        0, nullptr
    );

    /////////////////////////////////////////////////////////////////////
    // Begin render pass
    /////////////////////////////////////////////////////////////////////

    VkClearColorValue clearColorValue = { .float32 = { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkClearDepthStencilValue clearDepthStencilValue = { .depth = 1.0f, .stencil = 0 };

    VkClearValue clearValues[3] = {
        { .color = clearColorValue },
        { .depthStencil = clearDepthStencilValue },
        { .color = clearColorValue }
    };

    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = m_vulkanWindow->defaultRenderPass(),
        .framebuffer = m_vulkanWindow->currentFramebuffer(),
        .renderArea = { 
            .offset = { 0, 0 }, 
            .extent = { 
                static_cast<uint32_t>(swapChainSize.width()), 
                static_cast<uint32_t>(swapChainSize.height()) } 
            },
        .clearValueCount = static_cast<uint32_t>(m_vulkanWindow->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2),
        .pClearValues = clearValues
    };

    m_deviceFunctions->vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    /////////////////////////////////////////////////////////////////////
    // Map projection matrix
    /////////////////////////////////////////////////////////////////////
    
    Camera *camera = m_vulkanWindow->getCamera();

    QMatrix4x4 projectionMatrix = camera->getProjectionMatrix();
    QVector3D cameraPosition = camera->getPosition();

    // VkDeviceSize totalSize = UNIFORM_MATRIX_DATA_SIZE + UNIFORM_VECTOR_DATA_SIZE;
    VkDeviceSize offset = m_uniformBufferInfo[currentFrame].offset;

    // Copy projection matrix
    m_uniformBuffer.copyData(projectionMatrix.constData(), UNIFORM_MATRIX_DATA_SIZE, offset);

    // Copy camera position (after matrix)
    m_uniformBuffer.copyData(&cameraPosition, UNIFORM_VECTOR_DATA_SIZE, offset + UNIFORM_MATRIX_DATA_SIZE);


    /////////////////////////////////////////////////////////////////////
    // Bind pipeline
    /////////////////////////////////////////////////////////////////////

    m_deviceFunctions->vkCmdBindPipeline(commandBuffer, 
                                        VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                        m_pipeline);

    m_deviceFunctions->vkCmdBindDescriptorSets(commandBuffer, 
                                            VK_PIPELINE_BIND_POINT_GRAPHICS, 
                                            m_pipelineLayout, 
                                            0, 1,
                                            &m_descriptorSet[currentFrame], 
                                            0, nullptr);

    VkDeviceSize vertexBufferOffset = 0;
    VkBuffer vertexBuffer = m_vertexBuffer.getBuffer();
    m_deviceFunctions->vkCmdBindVertexBuffers(commandBuffer, 
                                            0, 1, 
                                            &vertexBuffer, 
                                            &vertexBufferOffset);


    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(swapChainSize.width()),
        .height = static_cast<float>(swapChainSize.height()),
        .minDepth = 0,
        .maxDepth = 1
    };

    m_deviceFunctions->vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = { 
            static_cast<uint32_t>(viewport.width), 
            static_cast<uint32_t>(viewport.height) 
        }
    };
    m_deviceFunctions->vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Calculate vertex count (each vertex has 8 elements: position, normal, and UV)
    // (x, y, z, x, y, z, u, v)
    const uint32_t vertexCount = sizeof(vertexData) / sizeof(vertexData[0]) / 8;  

    m_deviceFunctions->vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);

    m_deviceFunctions->vkCmdEndRenderPass(commandBuffer);

    m_renderTimeNs = m_renderTimer.nsecsElapsed();
    // double fps = 1e9/(static_cast<double>(m_renderTimeNs));

    // qDebug().nospace() << "Render time: " << (m_renderTimeNs / 1.0e6) << " ms, FPS: " << fps;

    m_vulkanWindow->frameReady();
    m_vulkanWindow->requestUpdate(); 
}

void VulkanRenderer::copyStorageImage(VkFence fence)
{
    // Wait for the provided fence to signal (ensuring prior work is complete)
    if (fence != VK_NULL_HANDLE)
    {
        VkResult result = m_deviceFunctions->vkWaitForFences(m_vulkanWindow->device(), 1, &fence, VK_TRUE, UINT64_MAX);
        if (result != VK_SUCCESS)
        {
            qWarning("Failed to wait for input fence (error code: %d)", result);
            return;
        }
    }

    VulkanCommandBuffer commandBuffer = VulkanCommandBuffer(m_vulkanWindow, m_graphicsCommandPool.getCommandPool(), m_graphicsQueue);

    commandBuffer.beginSingleTimeCommandBuffer();

    VkImage storageImage = m_vulkanWindow->getVulkanRayTracer()->getStorageImage(); 

    VkImageMemoryBarrier imageMemoryBarrierToTransferDst
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_renderImage.getImage(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };  

    m_deviceFunctions->vkCmdPipelineBarrier(
    commandBuffer.getCommandBuffer(),
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    0,
    0, nullptr,
    0, nullptr, 
    1, &imageMemoryBarrierToTransferDst);   

    VkImageCopy imageCopyRegion
    {
        .srcSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .srcOffset = {0, 0, 0},
        .dstSubresource = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .dstOffset = {0, 0, 0},
        .extent = {
            .width = render_width,
            .height = render_height,
            .depth = 1
        }
    };

    m_deviceFunctions->vkCmdCopyImage(
        commandBuffer.getCommandBuffer(), 
        storageImage, 
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
        m_renderImage.getImage(), 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        1, &imageCopyRegion);

    VkImageMemoryBarrier imageMemoryBarrierToShaderRead
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_renderImage.getImage(),
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };  

    m_deviceFunctions->vkCmdPipelineBarrier(commandBuffer.getCommandBuffer(),
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0,
    0, nullptr,
    0, nullptr, 
    1, &imageMemoryBarrierToShaderRead);   

    commandBuffer.endSubmitAndWait();
}
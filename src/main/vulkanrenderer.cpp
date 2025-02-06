#include "vulkanrenderer.h"
#include "vulkanwindow.h"
#include <QVulkanFunctions>
#include <QFile>
#include <QObject>
#include "camera.h"

static float vertexData[] = { // TODO: Include .obj loader and a button "Load .obj" to load vertexData
    // Front face (Z = 1.0f)
    -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // Bottom-left
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // Bottom-right
     0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // Top-right

     0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // Top-right
    -0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // Top-left
    -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 0.0f,  // Bottom-left

    // Back face (Z = -1.0f)
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Bottom-left
     0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Top-right
     0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Bottom-right

     0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Top-right
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Bottom-left
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // Top-left
    
    // Left face (X = -1.0f)
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,  // Bottom-front
    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  // Bottom-back
    -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  // Top-back

    -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 1.0f,  // Top-back
    -0.5f,  0.5f, -0.5f, 0.0f, 0.0f, 1.0f,  // Top-front
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,  // Bottom-front

    // Right face (X = 1.0f)
     0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // Bottom-front
     0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // Top-back
     0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // Bottom-back

     0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 0.0f,  // Top-back
     0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // Bottom-front
     0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,  // Top-front
     

    // Top face (Y = 1.0f)
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f,  // Back-left
     0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f,  // Front-right
     0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f,  // Back-right

     0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f,  // Front-right
    -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1.0f,  // Back-left
    -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f,  // Front-left

    // Bottom face (Y = -1.0f)
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // Back-left
     0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f,  // Back-right
     0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // Front-right

     0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // Front-right
    -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,  // Front-left
    -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f   // Back-left
};

static const int UNIFORM_MATRIX_DATA_SIZE = 16 * sizeof(float);
static const int UNIFORM_VECTOR_DATA_SIZE = 3 * sizeof(float);

static inline VkDeviceSize aligned(VkDeviceSize v, VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}

VulkanRenderer::VulkanRenderer(VulkanWindow *w)
    : m_window(w)
{   
    m_helper = new VulkanRendererHelper();

    const QList<int> counts = w->supportedSampleCounts();

    qDebug() << "Supported sample counts:" << counts;
    for (int s = 16; s >= 4; s /= 2) {
        if (counts.contains(s)) {
            qDebug("Requesting sample count %d", s);
            m_window->setSampleCount(s);
            break;
        }
    }
}

VkShaderModule VulkanRenderer::createShaderModule(const QString& filename)
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

void VulkanRenderer::initResources()
{
    QVulkanInstance *inst = m_window->vulkanInstance();
    VkDevice dev = m_window->device();
    m_devFuncs = m_window->vulkanInstance()->deviceFunctions(dev);

    VkResult result{};

    const int concurrentFrameCount = m_window->concurrentFrameCount(); 
    qDebug() << "Concurrent frame count:" << concurrentFrameCount;
    
    const VkPhysicalDeviceLimits *pdevLimits = &m_window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    qDebug("Uniform buffer offset alignment is %u", (uint) uniAlign);

    /////////////////////////////////////////////////////////////////////
    // Create buffers
    /////////////////////////////////////////////////////////////////////

    // Create vertex buffer
    VkBufferCreateInfo vertexBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(vertexData),
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &vertexBufferInfo, nullptr, &m_vertexBuffer);
    if (result != VK_SUCCESS)
        qFatal("Failed to create vertex buffer: %d", result);

    VkMemoryRequirements vertexBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_vertexBuffer, &vertexBufferMemoryRequirements);

    VkMemoryAllocateInfo vertexBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = vertexBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &vertexBufferMemoryAllocateInfo, nullptr, &m_vertexMemory);
    if (result != VK_SUCCESS)
        qFatal("Failed to allocate vertex memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_vertexBuffer, m_vertexMemory, 0);
    if (result != VK_SUCCESS)
        qFatal("Failed to bind vertex buffer memory: %d", result);

    // Create staging buffer
    VkBufferCreateInfo stagingBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = sizeof(vertexData),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    result = m_devFuncs->vkCreateBuffer(dev, &stagingBufferInfo, nullptr, &m_stagingBuffer);
    if (result != VK_SUCCESS)
        qFatal("Failed to create staging buffer: %d", result);

    VkMemoryRequirements stagingBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_stagingBuffer, &stagingBufferMemoryRequirements);

    VkMemoryAllocateInfo stagingBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &stagingBufferMemoryAllocateInfo, nullptr, &m_stagingMemory);
    if (result != VK_SUCCESS)
        qFatal("Failed to allocate staging memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_stagingBuffer, m_stagingMemory, 0);
    if (result != VK_SUCCESS)
        qFatal("Failed to bind staging buffer memory: %d", result);

    quint8 *pStaging;

    result = m_devFuncs->vkMapMemory(dev, m_stagingMemory, 0, stagingBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pStaging));
    if (result != VK_SUCCESS)
        qFatal("Failed to map staging memory: %d", result);

    memcpy(pStaging, vertexData, sizeof(vertexData));
    m_devFuncs->vkUnmapMemory(dev, m_stagingMemory);

    // Create uniform buffer
    const VkDeviceSize uniformBufferSize = aligned(UNIFORM_MATRIX_DATA_SIZE, uniAlign) + aligned(UNIFORM_VECTOR_DATA_SIZE, uniAlign);

    VkBufferCreateInfo uniformBufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = uniformBufferSize * concurrentFrameCount,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };
    
    result = m_devFuncs->vkCreateBuffer(dev, &uniformBufferInfo, nullptr, &m_uniformBuffer);
    if (result != VK_SUCCESS)
        qFatal("Failed to create uniform buffer: %d", result);

    VkMemoryRequirements uniformBufferMemoryRequirements;
    m_devFuncs->vkGetBufferMemoryRequirements(dev, m_uniformBuffer, &uniformBufferMemoryRequirements);

    VkMemoryAllocateInfo uniformBufferMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = uniformBufferMemoryRequirements.size,
        .memoryTypeIndex = m_window->hostVisibleMemoryIndex()
    };

    result = m_devFuncs->vkAllocateMemory(dev, &uniformBufferMemoryAllocateInfo, nullptr, &m_uniformMemory);
    if (result != VK_SUCCESS)
        qFatal("Failed to allocate uniform memory: %d", result);

    result = m_devFuncs->vkBindBufferMemory(dev, m_uniformBuffer, m_uniformMemory, 0);
    if (result != VK_SUCCESS)
        qFatal("Failed to bind uniform buffer memory: %d", result);

    quint8 *pUniform;
    result = m_devFuncs->vkMapMemory(dev, m_uniformMemory, 0, uniformBufferMemoryRequirements.size, 0, reinterpret_cast<void **>(&pUniform));
    if (result != VK_SUCCESS)
        qFatal("Failed to map uniform memory: %d", result);

    QMatrix4x4 ident;

    for (int i = 0; i < concurrentFrameCount; ++i) {
        const VkDeviceSize offset = i * uniformBufferSize;
        memcpy(pUniform + offset, ident.constData(), 16 * sizeof(float));

        m_uniformBufferInfo[i] = VkDescriptorBufferInfo{
            .buffer = m_uniformBuffer,
            .offset = offset,
            .range = uniformBufferSize
        };
    }
    m_devFuncs->vkUnmapMemory(dev, m_uniformMemory);

    /////////////////////////////////////////////////////////////////////
    // Pipeline shader stages
    /* More info...
    It uses .spv aliases defined in shaders.qrc
    All shaders are compiled automatically during CMake configuration with glslangValidator.exe
    */
    /////////////////////////////////////////////////////////////////////

    VkShaderModule vertShaderModule = createShaderModule(QStringLiteral(":/color_vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(QStringLiteral(":/color_frag.spv"));

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
        .pSpecializationInfo = nullptr
    },
    {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
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

    VkVertexInputBindingDescription vertexBindingDesc = {
        .binding = 0,
        .stride = 6 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        },
        { // color
            .location = 1,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 3 * sizeof(float)
        }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBindingDesc,
        .vertexAttributeDescriptionCount = 2,
        .pVertexAttributeDescriptions = vertexAttrDesc
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

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
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

    VkPipelineViewportStateCreateInfo viewportState = {
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

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
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

    VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .rasterizationSamples = m_window->sampleCountFlagBits(),
        .sampleShadingEnable = VK_FALSE,
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

    VkPipelineDepthStencilStateCreateInfo depthStencilState = {
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

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY, // Optional
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f} // Optional
    };

    /////////////////////////////////////////////////////////////////////
    // Pipeline dynamic state
    /* More info...
    While most of the pipeline state needs to be baked into the pipeline state, a limited amount of the state can actually be changed without recreating the pipeline at draw time. Examples are the size of the viewport, line width and blend constants. 

    This will cause the configuration of these values to be ignored and you will be able (and required) to specify the data at drawing time. This results in a more flexible setup and is very common for things like viewport and scissor state, which would result in a more complex setup when being baked into the pipeline state.
    */
    /////////////////////////////////////////////////////////////////////

    VkDynamicState dynamicStates[] = { 
        VK_DYNAMIC_STATE_VIEWPORT, 
        VK_DYNAMIC_STATE_SCISSOR 
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(VkDynamicState),
        .pDynamicStates = dynamicStates
    };
    
    /////////////////////////////////////////////////////////////////////
    // Set up descriptor set and its layout
    /////////////////////////////////////////////////////////////////////

    VkDescriptorPoolSize descPoolSizes = {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = uint32_t(concurrentFrameCount)
    };

    VkDescriptorPoolCreateInfo descPoolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .maxSets = static_cast<uint32_t>(concurrentFrameCount),
        .poolSizeCount = 1,
        .pPoolSizes = &descPoolSizes
    };

    result = m_devFuncs->vkCreateDescriptorPool(dev, &descPoolInfo, nullptr, &m_descPool);
    if (result != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d", result);

    VkDescriptorSetLayoutBinding layoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr
    };

    VkDescriptorSetLayoutCreateInfo descLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .bindingCount = 1,
        .pBindings = &layoutBinding
    };

    result = m_devFuncs->vkCreateDescriptorSetLayout(dev, &descLayoutInfo, nullptr, &m_descSetLayout);
    if (result != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d", result);

    for (int i = 0; i < concurrentFrameCount; ++i) {

        VkDescriptorSetAllocateInfo descSetAllocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descSetLayout
        };

        result = m_devFuncs->vkAllocateDescriptorSets(dev, &descSetAllocInfo, &m_descSet[i]);
        if (result != VK_SUCCESS)
            qFatal("Failed to allocate descriptor set: %d", result);

        VkWriteDescriptorSet descWrite = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = m_descSet[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pImageInfo = nullptr,
            .pBufferInfo = &m_uniformBufferInfo[i],
            .pTexelBufferView = nullptr
        };
        
        m_devFuncs->vkUpdateDescriptorSets(dev, 1, &descWrite, 0, nullptr);
    }

    /////////////////////////////////////////////////////////////////////
    // Pipeline layout
    /////////////////////////////////////////////////////////////////////

    VkPipelineCacheCreateInfo pipelineCacheInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .initialDataSize = 0,
        .pInitialData = nullptr
    };

    result = m_devFuncs->vkCreatePipelineCache(dev, &pipelineCacheInfo, nullptr, &m_pipelineCache);
    if (result != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d", result);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descSetLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };
    
    result = m_devFuncs->vkCreatePipelineLayout(dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d", result);

    /////////////////////////////////////////////////////////////////////
    // Graphics pipeline
    /////////////////////////////////////////////////////////////////////

    VkGraphicsPipelineCreateInfo pipelineInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pTessellationState = nullptr,
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = m_pipelineLayout,
        .renderPass = m_window->defaultRenderPass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    result = m_devFuncs->vkCreateGraphicsPipelines(dev, m_pipelineCache, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (result != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d", result);

    /////////////////////////////////////////////////////////////////////
    // Clean up
    /////////////////////////////////////////////////////////////////////

    if (vertShaderModule)
        m_devFuncs->vkDestroyShaderModule(dev, vertShaderModule, nullptr);
    if (fragShaderModule)
        m_devFuncs->vkDestroyShaderModule(dev, fragShaderModule, nullptr);

    /////////////////////////////////////////////////////////////////////
    // Logs
    /////////////////////////////////////////////////////////////////////

    QString info;
    info += QString::asprintf("Number of physical devices: %d\n", int(m_window->availablePhysicalDevices().count()));

    QVulkanFunctions *f = inst->functions();
    VkPhysicalDeviceProperties props;
    f->vkGetPhysicalDeviceProperties(m_window->physicalDevice(), &props);
    info += QString::asprintf("Active physical device name: '%s' version %d.%d.%d\nAPI version %d.%d.%d\n",
                              props.deviceName,
                              VK_VERSION_MAJOR(props.driverVersion), VK_VERSION_MINOR(props.driverVersion),
                              VK_VERSION_PATCH(props.driverVersion),
                              VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion),
                              VK_VERSION_PATCH(props.apiVersion));

    info += QStringLiteral("Supported instance layers:\n");
    for (const QVulkanLayer &layer : inst->supportedLayers())
        info += QString::asprintf("    %s v%u\n", layer.name.constData(), layer.version);
    info += QStringLiteral("Enabled instance layers:\n");
    for (const QByteArray &layer : inst->layers())
        info += QString::asprintf("    %s\n", layer.constData());

    info += QStringLiteral("Supported instance extensions:\n");
    for (const QVulkanExtension &ext : inst->supportedExtensions())
        info += QString::asprintf("    %s v%u\n", ext.name.constData(), ext.version);
    info += QStringLiteral("Enabled instance extensions:\n");
    for (const QByteArray &ext : inst->extensions())
        info += QString::asprintf("    %s\n", ext.constData());

    info += QString::asprintf("Color format: %u\nDepth-stencil format: %u\n",
                              m_window->colorFormat(), m_window->depthStencilFormat());

    info += QStringLiteral("Supported sample counts:");
    const QList<int> sampleCounts = m_window->supportedSampleCounts();
    for (int count : sampleCounts)
        info += QLatin1Char(' ') + QString::number(count);
    info += QLatin1Char('\n');

    emit static_cast<VulkanWindow *>(m_window)->vulkanInfoReceived(info);
}

void VulkanRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");

    emit m_helper->updateSwapChain();
}

void VulkanRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanRenderer::releaseResources()
{
    qDebug("releaseResources");

    VkDevice dev = m_window->device();

    if (m_pipeline) {
        m_devFuncs->vkDestroyPipeline(dev, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout) {
        m_devFuncs->vkDestroyPipelineLayout(dev, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_pipelineCache) {
        m_devFuncs->vkDestroyPipelineCache(dev, m_pipelineCache, nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if (m_descSetLayout) {
        m_devFuncs->vkDestroyDescriptorSetLayout(dev, m_descSetLayout, nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if (m_descPool) {
        m_devFuncs->vkDestroyDescriptorPool(dev, m_descPool, nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    if (m_vertexBuffer) {
        m_devFuncs->vkDestroyBuffer(dev, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }

    if (m_stagingBuffer) {
        m_devFuncs->vkDestroyBuffer(dev, m_stagingBuffer, nullptr);
        m_stagingBuffer = VK_NULL_HANDLE;
    }

    if (m_uniformBuffer) {
        m_devFuncs->vkDestroyBuffer(dev, m_uniformBuffer, nullptr);
        m_uniformBuffer = VK_NULL_HANDLE;
    }

    if (m_stagingMemory) {
        m_devFuncs->vkFreeMemory(dev, m_stagingMemory, nullptr);
        m_stagingMemory = VK_NULL_HANDLE;
    }

    if (m_uniformMemory) {
        m_devFuncs->vkFreeMemory(dev, m_uniformMemory, nullptr);
        m_uniformMemory = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::startNextFrame()
{
    VkDevice device = m_window->device();

    const QSize sz = m_window->swapChainImageSize();

    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();

    /////////////////////////////////////////////////////////////////////
    // Copy staging buffer to vertex buffer
    /////////////////////////////////////////////////////////////////////

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = sizeof(vertexData)
    };

    m_devFuncs->vkCmdCopyBuffer(cmdBuf, m_stagingBuffer, m_vertexBuffer, 1, &copyRegion);

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

    m_devFuncs->vkCmdPipelineBarrier(
        cmdBuf,
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

    VkClearColorValue clearColor = { .float32 = { 0.3f, 0.3f, 0.3f, 1.0f } };

    VkClearDepthStencilValue clearDS = { .depth = 1.0f, .stencil = 0 };

    VkClearValue clearValues[3] = {
        { .color = clearColor },
        { .depthStencil = clearDS },
        { .color = clearColor }
    };

    VkRenderPassBeginInfo rpBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = nullptr,
        .renderPass = m_window->defaultRenderPass(),
        .framebuffer = m_window->currentFramebuffer(),
        .renderArea = { .offset = { 0, 0 }, .extent = { static_cast<uint32_t>(sz.width()), static_cast<uint32_t>(sz.height()) } },
        .clearValueCount = static_cast<uint32_t>(m_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2),
        .pClearValues = clearValues
    };

    m_devFuncs->vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    /////////////////////////////////////////////////////////////////////
    // Map projection matrix
    /////////////////////////////////////////////////////////////////////
    
    quint8 *p;

    VkResult result = m_devFuncs->vkMapMemory(device, m_uniformMemory, m_uniformBufferInfo[m_window->currentFrame()].offset,
            UNIFORM_MATRIX_DATA_SIZE + UNIFORM_VECTOR_DATA_SIZE, 0, reinterpret_cast<void **>(&p));
    if (result != VK_SUCCESS)
        qFatal("Failed to map memory: %d", result);

    Camera *camera = m_window->getCamera();

    QMatrix4x4 m = camera->getProj();
    QVector3D pos = camera->getPos();

    memcpy(p, m.constData(), 16 * sizeof(float));
    memcpy(p + UNIFORM_MATRIX_DATA_SIZE, &pos, 3 * sizeof(float));

    m_devFuncs->vkUnmapMemory(device, m_uniformMemory);

    /////////////////////////////////////////////////////////////////////
    // Bind pipeline
    /////////////////////////////////////////////////////////////////////

    m_devFuncs->vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    m_devFuncs->vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1,
                               &m_descSet[m_window->currentFrame()], 0, nullptr);

    VkDeviceSize vbOffset = 0;
    m_devFuncs->vkCmdBindVertexBuffers(cmdBuf, 0, 1, &m_vertexBuffer, &vbOffset);

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(sz.width()),
        .height = static_cast<float>(sz.height()),
        .minDepth = 0,
        .maxDepth = 1
    };

    m_devFuncs->vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = { static_cast<uint32_t>(viewport.width), static_cast<uint32_t>(viewport.height) }
    };
    m_devFuncs->vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    const uint32_t vertexCount = sizeof(vertexData) / sizeof(vertexData[0]) / 6;  // Each vertex has 6 elements (x, y, z, r, g, b)

    m_devFuncs->vkCmdDraw(cmdBuf, vertexCount, 1, 0, 0);

    m_devFuncs->vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate(); 
    emit static_cast<VulkanWindow *>(m_window)->frameQueued();
}

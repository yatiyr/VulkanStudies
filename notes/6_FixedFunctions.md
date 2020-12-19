**Vertex Input**

We create VkPipelineVertexInputStateCreateInfo structure to describe format of vertex data that will be passed to the vertex shader. This is described in two ways;

   1. **Bindings**: spacing between data and whether the data is per-vertex or per-instance    (instancing is used for rendering multiple copies of the same mesh in a scene at once. it is used for rendering objects like trees and grass, see: https://en.wikipedia.org/wiki/Geometry_instancing)
   2. **Attribute descriptions:** type of the attributes passed to the vertex shader, which binding to load them from and at which offset.

We are hard coding vertex data right now, so we will implement this structure specifying there is no vertex data to load for now. This is going to be explained in **Vertex Buffer** chapter.

```c++
VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
vertexInputInfo.vertexBindingDescriptionCount = 0;
vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
vertexInputInfo.vertexAttributeDescriptionCount = 0;
vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
```

**Input Assembly**

VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry will be drawn from the vertices and if primitive restart should be enabled. The former is specified in the topology member and can have values like:

* VK_PRIMITIVE_TOPOLOGY_POINT_LIST    : points from vertices
* VK_PRIMITIVE_TOPOLOGY_LINE_LIST     : line from every 2 vertices without reuse
* VK_PRIMITIVE_TOPOLOGY_LINE_STRIP    : the end vertex of every line is used as start vertex for the next line
* VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : triangle from every 3 vertices without reuse
* VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle are used as first two vertices of the next triangle.

Normally vertices are loaded from the vertex buffer by index in sequential order but with an element buffer you can specify the indices to use yourself. This allows you to perform optimizations like reusing vertices. If you set the **primitiveRestart** enable member to **VK_TRUE**, then its possible to break up lines and triangles in the **_STRIP** topology modes by using a special index of **0xFFFF** or **0xFFFFFFFF** . For drawing a triangle, we use this info;

```c++
VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
inputAssembly.primitiveRestartEnable = VK_FALSE;
```

**Viewports and scissors**

A viewport basically describes the region of the framebuffer that the output will be rendered to. This will almost always be (0, 0) the (width, height).

```c++
VkViewport viewport{};
viewport.x = 0.0f;
viewport.y = 0.0f;
viewport.width = (float) swapChainExtent.width;
viewport.height = (float) swapChainExtent.height;
viewport.minDepth = 0.0f;
viewport.maxDepth = 1.0f;
```

Secondly, scissor rectangles define in which regions pixels will actually be stored. Any pixels outside the scissor rectangles will be discarded by the rasterizer. They function like a filter rather than a transformation. Here, we only want to draw to the entire framebuffer, so we'll specify a scissor rectangle that covers it entirely.

```c++
VkRect2D scissor{};
scissor.offset = {0, 0};
scissor.extent = _swapChainExtent;
```

after that, we combine viewport and scissor into a viewport state using **VkPipelineViewportStateCreateInfo** struct. It is possible to use multiple viewports and scissor rectangles on some graphics cards, so its members reference an array of them. Using multiple requires enabling a GPU feature.

```c++
VkPipelineViewportStateCreateInfo viewportState{};
viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
viewportState.viewportCount = 1;
viewportState.pViewports = &viewport;
viewportState.scissorCount = 1;
viewportState.pScissors = &scissor;
```

**Rasterizer**

Rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by fragment shader. It also performs **depth testing**, **face culling** and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering). All this is configured using the **VkPipelineRasterizationStateCreateInfo** structure.

```c++
VkPipelineRasterizationStateCreateInfo rasterizer{};
rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
rasterizer.depthClampEnable        = VK_FALSE;
rasterizer.rasterizerDiscardEnable = VK_FALSE;
rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
rasterizer.lineWidth               = 1.0f;
rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
rasterizer.depthBiasEnable         = VK_FALSE;
rasterizer.depthBiasConstantFactor = 0.0f;
rasterizer.depthBiasClamp          = 0.0f;
rasterizer.depthBiasSlopeFactor    = 0.0f;
```

**Multisampling**

We use **VkPipelineMultisampleStateCreateInfo** struct to configure multisampling, which is a way to perform **anti-aliasing**. It works by combining the fragment shader results of multiple polygons that rasterize to the same pixel. This mainly occurs along edges, which is also where the most noticeable aliasing artifacts occur.

Because it doesn't need to run the fragment shader multiple times if only one polygon maps to a pixel, it is significantly less expensive than simply rendering to a higher resolution and then downscaling. Enabling it requires enabling a GPU feature.

For now, we keep multisampling disabled.

```c++
VkPipelineMultisampleStateCreateInfo multisampling{};
multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
multisampling.sampleShadingEnable   = VK_FALSE;
multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
multisampling.minSampleShading      = 1.0f; // Optional
multisampling.pSampleMask           = nullptr; // Optional
multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
multisampling.alphaToOneEnable      = VK_FALSE; // Optional
```

**Depth and stencil testing**

If we are using a depth and/or stencil buffer, then we also need to configure the depth and stencil tests using **VkPipelineDepthStencilStateCreateInfo**. We don't have one right now, so we can simply pass a nullptr instead of a pointer to such a struct. We'll get back to it in the **depth buffering** chapter.

**Color Blending**

After a fragment shader has returned a color, it needs to be combined with the color that is already in the framebuffer. This transformation is known as color blending and there are two ways to do it.

* Mix the old and new value to produce a final color
* Combine the old and new value using bitwise operation

There are two types of structs to configure color blending. The first struct, **VkPipelineColorBlendAttachmentState** contains the configuration per attached framebuffer and the second struct, **VkPipelineColorBlendStateCreateInfo** contains the global color blending settings. In our case we only have one framebuffer:

```c++
VkPipelineColorBlendAttachmentState colorBlendAttachment{};
colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT;
colorBlendAttachment.blendEnable         = VK_FALSE;
colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;
colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;
```

and we create **VkPipelineColorBlendStateCreateInfo** struct;

```c++
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable     = VK_FALSE;
        colorBlending.logicOp           = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount   = 1;
        colorBlending.pAttachments      = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;
```

**Dynamic state**

A limited amount of the state that we've specified in the previous structs can actually be changed without recreating the pipeline. Examples are the size of the viewport, line width and blend constants. If we want to do that, we have to fill in a **VkPipelineDynamicStateCreateInfo** structure;

```c++
VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_LINE_WIDTH
};

VkPipelineDynamicStateCreateInfo dynamicState{};
dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
dynamicState.dynamicStateCount = 2;
dynamicState.pDynamicStates    = dynamicStates;
```

**Pipeline layout**

You can use uniform values in shaders, which are globals similar to dynamic state variables that can be changed at drawing time to alter the behavior of your shaders without having to recreate them. They are commonly used to pass the transformation matrix to the vertex shader, or to create texture samplers in the fragment shader.

These uniform values need to be specified during pipeline creation by creating a **VkPipelineLayout** object. Even though we won't be using them until future chapters, we are still required to create an empty pipeline layout.

We create a class member to hold this object, because we will refer to it from other functions at a later point in time

```c++
VkPipelineLayout _pipelineLayout;
```

and we create the object;

```c++
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 0; // Optional
pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout!");
}
```

and we destroy it at cleanup;

```c++
void cleanup() {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    ...
}
```


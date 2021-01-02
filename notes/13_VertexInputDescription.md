We are going to change hardcoded vertex data shader with vertex buffer in memory. We will first create a CPU visible buffer and then we will see staging buffers to copy vertex data to high performance memory.

We first change **shader.vert** 

```c++
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

we now introduce glm to implement vertex data in our program to pass it to our shader.vert

```c++
#include <glm/glm.hpp>

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

// We specify a vector of vertices
const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
```

we have to tell Vulkan how to pass this data format to vertex shader once it's been uploaded into GPU memory. There are two types of structures needed to convey this information.

we add a new function to struct **Vertex** called getBindDescription to populate it with the right data. And then we implement another function called getAttributeDescriptions{}.

```c++
struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription() {

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding   = 0;
        bindingDescription.stride    = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding  = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset   = offsetof(Vertex, pos);

        attributeDescriptions[1].binding  = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset   = offsetof(Vertex, color);
        
        return attributeDescriptions;
    }
};
```
**Binding Descriptions**

a vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.


All of our per-vertex data is packed together in one array, so we're only going to have one binding. The binding parameter specifies the index of the binding in the array of bindings. The stride parameter specifies the number of bytes from one entry to the next, and the inputRate parameter can have one of the following values:

* VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
* VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance

We're not going to use instanced rendering, so we'll stick to per-vertex data.

**Attribute Descriptions**

Secondly we implement attribute descriptions. As the function prototype indicates, there are going to be two of these structures. An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data originating from a binding description. We have two attributes, position and color, so we need two attribute description structs.

The binding parameter tells Vulkan from which binding the per-vertex data comes. The location parameter references the location directive of the input in the vertex shader. The input in the vertex shader with location 0 is the position, which has two 32-bit float components.

The format parameter describes the type of data for the attribute. A bit confusingly, the formats are specified using the same enumeration as color formats. The following shader types and formats are commonly used together:

* float: VK_FORMAT_R32_SFLOAT
* vec2: VK_FORMAT_R32G32_SFLOAT
* vec3: VK_FORMAT_R32G32B32_SFLOAT
* vec4: VK_FORMAT_R32G32B32A32_SFLOAT

As you can see, you should use the format where the amount of color channels matches the number of components in the shader data type. It is allowed to use more channels than the number of components in the shader, but they will be silently discarded. If the number of channels is lower than the number of components, then the BGA components will use default values of (0, 0, 1). The color type (SFLOAT, UINT, SINT) and bit width should also match the type of the shader input. See the following examples:

* ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
* uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
* double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float

The format parameter implicitly defines the byte size of attribute data and the offset parameter specifies the number of bytes since the start of the per-vertex data to read from. The binding is loading one Vertex at a time and the position attribute (pos) is at an offset of 0 bytes from the beginning of this struct. This is automatically calculated using the offsetof macro.

**Modifying createGraphicsPipeline function**

we modifiy our createGraphicsPipeline function according to changes that we've made.

```c++
void createGraphicsPipeline() {
    auto vertShaderCode = readFile("../shaders/vert.spv");
    auto fragShaderCode = readFile("../shaders/frag.spv");

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);


    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    ...
}
```

if we run the program now, validation layers will complain because we didn't create vertex buffers. It will be created next time.
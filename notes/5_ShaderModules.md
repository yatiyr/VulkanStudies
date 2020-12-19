Shader code is specified in *bytecode* format unlike other API's. This format is called **SPIR-V** and designed for Vulkan and OpenCL. We can write graphics and compute shaders.

Using bytecode is more advantageous because it is easier to turn it into native code. 

Human readable syntax like GLSL may have vendor specific implementations. This may cause conflicts and compile errors. This avoided with **SPIR-V**.

We don't need to write **SPIR-V** ourselves. Khronos has a compiler which turns **GLSL** into **SPIR-V**. This compiler produces **SPIR-V** binaries that we can use in our programs.

We can also include the compiler as alibrary to produce **SPIR-V** format.

*glslangValidator.exe* can be used but this tutorial uses *glslc.exe* by Google. The advantage of this is that it uses same parameter format of gcc, clang and includes some extra functionality like includes. They are all included in **Vulkan SDK**.

GLSL is a shading language with a C-style syntax. It has a main function that is invoked for every object. Instead of using parameters for input and return value as output, GLSL uses global variables to handle input and output. Language includes many features to aid in graphics programming, like built-in vector and matrix primitives. Functions for operations like cross products, matrix-vector products and reflections around a vector are included.

Vector type is called vec with anumber indicating the amount of elements. E.g, vec3 is a 3D vector. We can create a new vector from multiple components at the same time.

Vertex shader processes each incoming vertex. It takes world position, color, normal and texture coordinates as input. Output is the final position in clip coordinates and the attributes that need to be passed to fragment shader like color and texture coordinates. These values will then be interpolated over the fragments by the rasterizer to produce a smooth gradient.

We create 2 files named shader.vert and shader.frag inside a folder named shaders and compile them using glslc

shader.vert

```c++
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 fragColor;

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```

shader.frag

```c++
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}
```

and compile.sh for compiling these into spir-v easily

```sh
#!/bin/sh

glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv
```

for loading compiled shader code to our vulkan program, we create a helper function like

```c++
static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if(!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}
```

we also need to wrap what we read into VkShaderModule  object. We also create another helper function for that.

```c++
VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader model!");
    }

    return shaderModule;
}
```

at the and createGraphicsPipeline function turns into this.

```c++
    void createGraphicsPipeline() {
        auto vertShaderCode = readFile("../shaders/vert.spv");
        auto fragShaderCode = readFile("../shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VKShaderModule fragShaderModule = createShaderModule(fragShaderCode);


        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pNext  = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
```
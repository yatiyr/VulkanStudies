# DESCRIPTOR LAYOUT AND BUFFER

We need model-view-projection matrices and we have to update data for changing transformations. For that, vulkan uses **resource descriptors**. 

Descriptor is a way for shaders to freely access resources like buffers and images. We will set up a buffer that contains the transformation matrices and have the vertex shader access them through a descriptor. There are three parts in usage of descriptors.

* Specify a descriptor layout during pipeline creation
* Allocate a descriptor set from a descriptor pool
* Bind the descriptor set during rendering

The descriptor layout specifies the types of resources that are going to be accessed by the pipeline, just like a render pass specifies the types of attachments that will be accessed. A **descriptor set** specifies the actual buffer or image resources that will be bound to the descriptors, just like a framebuffer specifies the actual image views to bind to render pass attachments. The descriptor set is then bound for the drawing commands just like the vertex buffers and framebuffer.

There are many types of descriptors. We will work on Uniform Buffer Objects (UBO). We want the vertex shader to have in a C struct like this.

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

Then we can copy the data to a VkBuffer and access it through a uniform buffer object descriptor from the vertex shader like this:

```c++
layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}

```

We're going to update the model, view and projection matrices every frame to make the rectangle from the previous chapter spin around in 3D.

**Vertex Shader**

We modify the vertex shader to include the uniform buffer object.

```c++
#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
}
```

Order of uniform, in and out declarations doesn't matter. The binding directive is similar to the location directive. We will reference thsi binding in the descriptor layout. The line with gl_Position is changed to use the transformations to compute the final position in clip coordinates. Unlike the 2D triangles, the last component of the clip coordinates may not be 1, which will result in a division when converted to the final normalized device coordinates on the screen. This is used in perspective projection as the perspective division and is essential for making closer objects look larger than objects that are further away.

We will now define UBO on the C++ side and to tell Vulkan about this descriptor in the vertex shader.

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

We can exactly match the definition in the shader using data types in GLM. The data in the matrices is binary compatible with the way the shader expects it, so we can later just memcpy a UniformBufferObject to a VkBuffer.

We need to provide details about every descriptor binding used in the shaders for pipeline creation, just like we had to do for every vertex attribute and its location index. We'll set up a new function to define all of this information called createDescriptorSetLayout. It should be called right before pipeline creation, because we're going to need it there.

```c++
void initVulkan() {
    ...
    createDescriptorSetLayout();
    createGraphicsPipeline();
    ...
}

...

void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding         = 0;
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;

    uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings    = &uboLayoutBinding;

    if(vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}
```

The first two fields specify the binding used in the shader and the type of descriptor, which is a uniform buffer object. It is possible for the shader variable to represent an array of uniform buffer objects, and descriptorCount specifies the number of values in the array. This could be used to specify a transformation for each of the bones in a skeleton for skeletal animation, for example. Our MVP transformation is in a single uniform buffer object, so we're using a descriptorCount of 1.

We also need to specify in which shader stages the descriptor is going to be referenced. The stageFlags field can be a combination of VkShaderStageFlagBits values or the value VK_SHADER_STAGE_ALL_GRAPHICS. In our case, we're only referencing the descriptor from the vertex shader.

The pImmutableSamplers field is only relevant for image sampling related descriptors, which we'll look at later. We leave it nullptr for now.

All of the descriptor bindings are combined into a single VkDescriptorSetLayout object. We define two member variables

```c++
VkDescriptorSetLayout _descriptorSetLayout;
```

We can then create it using vkCreateDescriptorSetLayout.

We also modify VkPipelineLayout creation in createGraphicsPipeline() function like this to include descriptorSetLayout

```c++VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
pipelineLayoutInfo.setLayoutCount = 1;
pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
```

After all it also needs to be destroyed. We destroy it like this in cleanup();

```c++
void cleanup() {
    cleanupSwapChain();

    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    ...
}
```

**Uniform Buffer**

We have to create buffer that contains UBO data for the shader, but we need to create this buffer first. We're going to copy new data to the uniform buffer every frame, so it doesn't really make any sense to have a staging buffer. It would just add extra overhead in this case and likely degrade performance instead of improving it.

We should have multiple buffers, because multiple frames may be in flight at the same time and we don't want to update the buffer in preparation of the next frame while a previous one is still reading from it. We could either have a uniform buffer per from or per swap chain image. However, since we need to refer to the uniform buffer from the command buffer that we have per swap chain image, it makes the most sense to also have a uniform buffer per swap chain image.

We add new class members for uniformBuffers and uniformBuffersMemory

```c++
std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
```

we implement createUniformBuffers() function. There is no vkMapMemory in this function because we are going to write a separate function that updates the uniform buffer with a new transformation every frame;

```c++
void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    _uniformBuffers.resize(swapChainImages.size());
    _uniformBuffersMemory.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffers[i], _uniformBuffersMemory[i]);
    }
}
```

we will lean it in cleanupSwapChain() because uniform data will be used for all draw calls and it depends on number of swap chain images. Uniform data should only be destroyed when we stop rendering.

```c++
void cleanupSwapChain() {
    ...

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
        vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
    }
}
```

we also need to recreate it in recreateSwapChain;

```c++
void recreateSwapChain() {
    ...

    createFramebuffers();
    createUniformBuffers();
    createCommandBuffers();
}
```

create a new function updateUniformBuffer and add a call to it from drawFrame right after we know which swap chain image we're going to acquire;

```c++
void drawFrame() {
    ...

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    ...

    updateUniformBuffer(imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    ...
}
```

we add new includes;

```c++
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
```

we implement updateUniformBuffer;

```c++
void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f,0.0f,1.0f));
    ubo.view  = glm::lookAt(glm::vec3(2.0f,2.0f,2.0f), glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,0.0f,1.0f));
    ubo.proj  = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float) _swapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(_device, _uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(_device, _uniformBuffersMemory[currentImage]);
}
```


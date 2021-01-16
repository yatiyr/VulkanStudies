# DESCRIPTOR POOL AND SETS

The descriptor layout from the previous chapter describes the type of descriptors that can be bound. In this chapter we are going to create a descriptor set for each VkBuffer resource to bind it to the uniform buffer descriptor.

**Descriptor Pool**

Descriptor sets can't be created directly, they must be allocated from a pool like command buffers. The equivalent for descriptor sets is unsurprisingly called a descriptor pool. We'll write a new function createDescriptorPool to set it up.

```c++
void initVulkan() {
    ...
    createUniformBuffers();
    createDescriptorPool();
    ...
}
```

we implement createDescriptorPool() function;

```c++
void createDescriptorPool() {

    VkDescriptorPoolSize poolSize{};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(_swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = static_cast<uint32_t>(_swapChainImages.size());
    poolInfo.flags         = 0;

    if(vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

}
```

In this function firstly, we describe which descriptor types our descriptor sets are going to contain and how many of them, using VkDescriptorPoolSize structures

And then we will allocate one of these descriptors for every frame. This pool size structure is referenced by the main VkDescriptorPoolCreateInfo

Aside from the maximum number of individual descriptors that are available, we also need to specify the maximum number of descriptor sets that may be allocated.

The structure has an optional flag similar to command pools that determines if individual descriptor sets can be freed or not. VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. We are not going to touch the descriptor set after creating it, so we don't need this flag. You can leave flags to its default value of 0.

we add new class member;

```c++
VkDescriptorPool _descriptorPool;
```

we need to cleanup pool in clenupSwapChain() because we need to destroy it when swap chain is recreated.

```c++
void cleanupSwapChain() {
    ...

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}
```
And recreated in swapChain();

```c++
void recreateSwapChain() {
    ...

    createUniformBuffers();
    createDescriptorPool();
    createCommandBuffers();
}
```

**Descriptor Set**

We can now allocate the descriptor sets themselves. Add createDescriptorSets function for that purpose.

```c++
void initVulkan() {
    ...
    createDescriptorPool();
    createDescriptorSets();
    ...
}

void recreateSwapChain() {
    ...
    createDescriptorPool();
    createDescriptorSets();
    ...
}
```

we now implement createDescriptorSets() function;

```c++
void createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(_swapChainImages.size(), _descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(_swapChainImages.size());
    allocInfo.pSetLayouts        = layouts.data();

    _descriptorSets.resize(_swapChainImages.size());
    if(vkAllocateDescriptorSets(_device, &allocInfo, _descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for(size_t i=0; i<_swapChainImages.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet           = _descriptorSets[i];
        descriptorWrite.dstBinding       = 0;
        descriptorWrite.dstArrayElement  = 0;
        descriptorWrite.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount  = 1;
        descriptorWrite.pBufferInfo      = &bufferInfo;
        descriptorWrite.pImageInfo       = nullptr;
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
    }

}
```

A descriptor set allocation is described with a VkDescriptorSetAllocateInfo struct. We need to specify the descriptor pool to allocate from, the number of descriptor sets to allocate, and the descriptor layout to base them on

In our case, we will create on descriptor set for each swap chain image, all with the same layout. Unfortunately we do need all the copies of the layout because the next function expects an array matching the number of sets.

We add a class member to hold the descriptor set handles and allocate them with vkAllocateDescriptorSets.

```c++
std::vector<VkDescriptorSet> _descriptorSets;
```

We don't need to clear descriptor sets explicitly, because they will be automatically freed when the descriptor pool is destroyed. The call to vkAllocateDescriptorSets will allocate descriptor sets, each with one uniform buffer descriptor.

The descriptor sets have been allocated now, but the descriptors within still need to be configured. We'll now add a loop to populate every descriptor.

Descriptors that refer to buffers, like our uniform buffer descriptor, are configured with a VkDescriptorBufferInfo struct. This structure specifies the buffer and region within it that contains the data for the descriptor.

If we're overwriting the whole buffer, like we are in this case, then it is also possible to use the VK_WHOLE_SIZE value for the range. The configuration of descriptors is updated using to vkUpdateDescriptorSets function, which takes an array of VkWriteDescriptorSet structs as parameter.

The first two fields in descriptorWrite, specify the descriptor set and the binding. We gave our uniform buffer binding index 0. Remember that descriptors can be arrays, so we also need to specify the first index in the array that we want to update. We're not using an array, so the index is simply 0.

We need to specify the type of descriptor again. It is possible to update multiple descriptors at one in an array, starting at index dstArrayElement. The descriptor count field specifies how many array elements you want to update.

The last field references an array with descriptorCount structs that actually configure the descriptors. It depends on the type of descriptor which one of the three we actually need to use. The pBufferInfo field is used for descriptors that refer to buffer data, pImageInfo is used for descriptors that refer to image data, and pTexelBufferView is used for descriptors that refer to buffer views. Our descriptor is based on buffers, so we're using pBufferInfo.

The updates are applied using vkUpdateDescriptorSets. It accepts two kinds of arrays as parameters: an array of VkWriteDescriptorSet and an array of VkCopyDescriptorSet. The latter can be used to copy descriptors to each other, as its name implies.

**Using Descriptor Sets**

We now need to update the createCommandBuffers function to actually bind the right descriptor set for each swap chain image to the descriptors in the shader with vkCmdBindDescriptorSets. This needs to be done before the vkCmdDrawIndexed call.

```c++
vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
```

Unline vertex and index buffers, descriptor sets are not unique to graphics pipelines. Therefore we need to specify if we want to bind descriptor sets to the graphics or compute pipeline. The next parameter is the layout that the descriptors are based on. The next three parameters specify the index of the first descriptor set, the number of sets to bind, and the array of sets to bind. We'll get back to this in a moment. The last two parameters specify an array of offsets that are used for dynamic descriptors. We'll look at these in a future chapter.

If we run the program now, we will notice that nothing is there. It is because of the Y-flip we did in the projection matrix, the vertices are now being drawn in counter clockwise order instead of clockwise order. This causes backface culling to kick in and prevents any geometry from being drawn. We go to the createGraphicsPipeline function and modify the frontFace in VkPipelineRasterizationStateCreateInfo to correct this;

```c++
rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
```
**Alignment Requirements**

Data in the c++ structure should match with the uniform definition in the shader. It seems obvious enough to simply use the same types in both.

```c++
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

however, that's not all there is to it. For example, try modifying the struct and shader to look like this;

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

layout(binding = 0) uniform UniformBufferObject {
    vec2 foo;
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;
```

If we recompile this, our shape is going to disappear because c++ struct must be aligned. Some Alignment requirements are like this;

* Scalars have to be aligned by N(=4 bytes given 32 bit floats)
* A vec2 must be aligned by 2N(= 8 bytes)
* A vec3 or vec4 must be aligned by 4N (=16 bytes)
* A nested structure must be aligned by the base alignment of its members rounded up to a multiple of 16
* mat4 matrix must have the same alignment as a vec4

Our original shader with just three mat4 fields already meet the alignment requirements. As each mat4 is 4 x 4 x 4 = 64 bytes in size, model has an offset of 0, view has an offset of 64 and proj has an offset of 128. All of these are multiples of 16 and that's why it worked fine.

The new structure starts with a vec2 which is only 8 bytes in size and therefure throws off all of the offsets. Now model has an offset of 8, view an offset of 72 and proj an offset of 136, none of which are multiples of 16. To fix this problem we can use the alignas specifier introduced in c++11

```c++
struct UniformBufferObject {
    glm::vec2 foo;
    alignas(16) glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};
```

If you now compile and run your program again you should see that the shader correctly receives its matrix values once again.

Luckily we can get away with using alignas by definining these;

```c++
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/glm.hpp>
```
but being explicit is better because this is going to break if we use nested structures.

```c++
struct Foo {
    glm::vec2 v;
};

struct UniformBufferObject {
    Foo f1;
    Foo f2;
};
```

shader;

```c++
struct Foo {
    vec2 v;
};

layout(binding = 0) uniform UniformBufferObject {
    Foo f1;
    Foo f2;
} ubo;
```
in this case f2 will have an offset of 8 whereas it should have an offset of 16 since it is a nested structure. In this case you must specify the alignment yourself.

```c++
struct UniformBufferObject {
    Foo f1;
    alignas(16) Foo f2;
};
```
So it is better to be explicit in everthing about alignment. That way it is easier to identify errors.

```c++
struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
```
**Multiple descriptor sets**

As some of the structures and function calls hinted at, it is actually possible to bind multiple descriptor sets simulatneously. You need to specify a descriptor layout for each descriptor set when creating the pipeline layout. Shaders can then reference specific descriptor sets like this.

```c++
layout(set = 0, binding = 0) uniform UniformBufferObject { ... }
```

We can use this feature to put descriptors that vary per-object and descriptors that are shared into seperate descriptor sets. In that case you avoid rebinding most of the descriptors across draw calls which is potentially more efficient.
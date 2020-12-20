We haven't created framebuffer yet. We first create a class member,

```c++
std::vector<VkFramebuffer> swapChainFramebuffers;
```

and create a new function **createFramebuffers()** right after creating graphics pipeline in **initVulkan()** function.

```c++
void createFramebuffers() {
    _swapChainFramebuffers.resize(_swapChainImageViews.size());

    for(size_t i=0; i<_swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            _swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = _renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = _swapChainExtent.width;
        framebufferInfo.height          = _swapChainExtent.height;
        framebufferInfo.layers          = 1;

        if(vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }

    }
}
```

and we destroy it in cleanup;

```c++
void cleanup() {
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    ...
}
```
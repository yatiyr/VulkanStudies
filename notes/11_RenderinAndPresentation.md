Now we are actually writing functions to draw the triangle.

We will implement a **drawFrame()** function in main loop.

```c++
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        drawFrame();
    }
}

...

void drawFrame() {

}
```

**drawFrame()** function will;

* Acquire an image from swap chain
* Execute the command buffer with that image as attachment in framebuffer
* Return the image to swap chain for presentation

Each of these events are executed asynchronously. But they depend on eachother. In order to avoid undefined return order, we can introduce fences or semaphores.

Difference between semaphore and fences is that the state of fences can be accessed from our program using calls like **vkWaitForFences** and semaphores cannot be. Fences are mainly designed to synchronize our application itself with rendering operation, whereas semaphores are used to synchronize operations within or across command queues. We cant to synchronize the queue operations of draw commands and presentation, which makes semaphores the best fit.

**Semaphores**

We need one semaphore to signal an image has been acquired and is ready for rendering, and another one to signal that rendering has finished and presentation can happen. We create two class members to store these semaphore objects.

```c++
VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
```

we then add **createSemaphores()** function to **initVulkan()** as usual. This is the last one

```c++
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffers();
    createSemaphores();
}

...

void createSemaphores() {

}
```

Creating semaphores is like creating all the other things in this tutorial. We have to fill a struct named **VkSemaphoreCreateInfo**

```c++
void createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphore) != VK_SUCCESS
    || vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores!");
    }
}
```

I WILL CARRY ON WRITING THIS.
Window size can change so that swap chain may be incompatible. We have to handle events like this.

We create a new function which calls these functions

```c++
void recreateSwapChain() {


    int width = 0, height = 0;
    glfwGetFramebufferSize(_window, &width, &height);
    while(width == 0 || height == 0) {
        glfwGetFramebufferSize(_window,&width,&height);
        glfwWaitEvents();
    }


    vkDeviceWaitIdle(_device);
    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
}
```

we add a cleanup function for swapChain

```c++
void cleanupSwapChain() {
    for(size_t i=0; i<_swapChainFramebuffers.size(); i++) {
        vkDestroyFramebuffer(_device,_swapChainFramebuffers[i],nullptr);
    }

    vkFreeCommandBuffers(_device,_commandPool,static_cast<uint32_t>(_commandBuffers.size()),_commandBuffers.data());
    vkDestroyPipeline(_device,_graphicsPipeline,nullptr);
    vkDestroyPipelineLayout(_device,_pipelineLayout,nullptr);
    vkDestroyRenderPass(_device,_renderPass,nullptr);

    for(size_t i = 0; i<_swapChainImageViews.size(); i++) {
        vkDestroyImageView(_device,_swapChainImageViews[i],nullptr);
    }

    vkDestroySwapchainKHR(_device,_swapChain,nullptr);
}
```

we use it in cleanup to

```c++
void cleanup() {

    cleanupSwapChain();

    for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_device, _inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(_device, _commandPool, nullptr);
    vkDestroyDevice(_device, nullptr);
        
    if(enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);

    glfwDestroyWindow(_window);
    glfwTerminate();
}
```
we also change init window accordingly

```c++
void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    _window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(_window, this);
    glfwSetFramebufferSizeCallback(_window, framebufferResizeCallback);
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}
```
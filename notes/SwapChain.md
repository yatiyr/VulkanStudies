**THINGS I'VE LEARNED**

**1.** Swap chain is a queue of images waiting to be presented.

**2.** We have to look for swapchain support in gpus and enable **VK_KHR_swapchain** extension

**3.** Like validation layer, we create a device extension list;

```c++

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

```

**4.**  New function is added to class about checking deviceExtensionSupport and rateDeviceSuitability function changed accordingly;

```c++

    int rateDeviceSuitability(VkPhysicalDevice device) {

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    

        int score = 0;

        // Discrete GPUS have a significant performance advantage
        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;


        if(!deviceFeatures.geometryShader) {
            return 0;
        }

        QueueFamilyIndices indices = findQueueFamilies(device);
        if(!indices.isComplete()) {
            return 0;
        }

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        if(!extensionsSupported) {
            return 0;
        }
        

        return score;
    }
```
and checkDeviceExtensionSupport function;

```c++
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for(const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }
```

**5.** We need to enable VK_KHR_swapchain extension, we add these into createLogicalDevice function;

```c++
createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
createInfo.ppEnabledExtensionNames = deviceExtensions.data();
```
**6.** We checket for the availability of swap chain but it may be incompatible with window surface. We need to check;
1. Basic surface capabilities(min/max number of images in swap chain, min/max width and height of images)
2. Surface formats(pixel format, color space)
3. Available presentation modes

so like findQueueFamilies we use a struct to pass details;

```c++
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
```

and we add a function to populate this struct;

```c++
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {

        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);
        
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

        if(formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
        }

        uint presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

        if(presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
        }

        

        return details;
    }
```

**7.** We extend rateDeviceSuitability and check swap chain is adequate.

```c++
    int rateDeviceSuitability(VkPhysicalDevice device) {

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    

        int score = 0;

        // Discrete GPUS have a significant performance advantage
        if(deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }

        // Maximum possible size of textures affects graphics quality
        score += deviceProperties.limits.maxImageDimension2D;


        if(!deviceFeatures.geometryShader) {
            return 0;
        }

        QueueFamilyIndices indices = findQueueFamilies(device);
        if(!indices.isComplete()) {
            return 0;
        }

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        if(!extensionsSupported) {
            return 0;
        }

        bool swapChainAdequate = false;
        if(extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() &&
                                !swapChainSupport.presentModes.empty();

            if(!swapChainAdequate) {
                return 0;
            }
        }

        return score;

    }
```

**8.** We need to choose right settings. There are three types of settings to determine;
1. Surface format (color depth)
2. Presentation mode (conditions for "swapping" images to the screen)
3. Swap extent (resolution of images in swap chain)
   
we have to find best values for these settings. we add the logic for that.

for surface format we add this;

```c++
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

        for(const auto& availableFormat : availableFormats) {
            if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
               availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                   return availableFormat;
               }
        }

        return availableFormats[0];
    }
```

for presention there are four modes;
1. VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by the application are transferred to the screen right away, which may result in tearing
   
2. VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue.If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games. The moment that the display is refreshed is known as "vertical blank"
   
3. VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank. Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
   
4. VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones. This mode can be used to implement triple buffering, which allows you to avoid tearing with significantly less latency issues than standard vertical sync that uses double buffering.

we write function to select presentation mode, we look for VK_PRESENT_MODE_MAILBOX_KHR first because it has less latency and avoids tearing;

```c++
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        
        for(const auto& availablePresentMode : availablePresentModes) {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }
```

for swap extent;

The swap extent is the resolution of the swap chain images and it's almost always exactly equal to the resolution of the window that we're drawing to in pixels (more on that in a moment). The range of the possible resolutions is defined in the VkSurfaceCapabilitiesKHR structure. Vulkan tells us to match the resolution of the window by setting the width and height in the currentExtent member. However, some window managers do allow us to differ here and this is indicated by setting the width and height in currentExtent to a special value: the maximum value of uint32_t. In that case we'll pick the resolution that best matches the window within the minImageExtent and maxImageExtent bounds. But we must specify the resolution in the correct unit.

GLFW uses two units when measuring sizes: pixels and screen coordinates. For example, the resolution {WIDTH, HEIGHT} that we specified earlier when creating the window is measured in screen coordinates. But Vulkan works with pixels, so the swap chain extent must be specified in pixels as well. Unfortunately, if you are using a high DPI display (like Apple's Retina display), screen coordinates don't correspond to pixels. Instead, due to the higher pixel density, the resolution of the window in pixel will be larger than the resolution in screen coordinates. So if Vulkan doesn't fix the swap extent for us, we can't just use the original {WIDTH, HEIGHT}. Instead, we must use glfwGetFramebufferSize to query the resolution of the window in pixel before matching it against the minimum and maximum image extent.

choosing swap extent is like this;

```c++
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if(capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }
```

**9.** For creating swap chain, we implement a function createSwapChain(), we first add these member variables into the class;

```c++
VkSwapchainKHR _swapChain;
std::vector<VkImage> _swapChainImages;
VkFormat _swapChainImageFormat;
VkExtent2D _swapChainExtent;
```
and we implement createSwapChain();
```c++
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = _surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                         indices.presentFamily.value()};

        if(indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if(vkCreateSwapchainKHR(_device,&createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
        _swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

        _swapChainImageFormat = surfaceFormat.format;
        _swapChainExtent = extent;

    }

```




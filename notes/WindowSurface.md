**THINGS THAT I'VE LEARNED**

**1.** In order to establish connection between Vulkan and Window System, we need to use WSI(Window System Integration) extensions
**2.** WSI is already enabled because it is included in list returned by *glfwGetRequiredInstanceExtensions*
**3.** Added *VkSurfaceKHR _surface* class member
**4.** In windows, surface creation is like this
```c++
VkWin32SurfaceCreateInfoKHR createInfo{};
createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
createInfo.hwnd = glfwGetWin32Window(window);
createInfo.hinstance = GetModuleHandle(nullptr);

if (vkCreateWin32SurfaceKHR(_instance, &createInfo, nullptr, &_surface) != VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
}
```
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;and it is different for each platform
**5.** glfwCreateWindowSurface performs thing we do in ***4***  for each platform implicitly
**6.** created createSurface() function and called it in initVulkan(), create surface;
```c++
void createSurface() {
    if (glfwCreateWindowSurface(_instance, window, nullptr, &_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}
```

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;and destroyed that surface at cleanup;

```c++
void cleanup() {
        if(enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
        }

        vkDestroyDevice(_device, nullptr);
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyInstance(_instance, nullptr);
        glfwDestroyWindow(_window);
        glfwTerminate();
    }
```

**7.** We also need to check whether our device supports window system integration. Problem is queue specific, so we need to find queue family for presenting. We change ***sturct QueueFamilyIndices*** like this;

```c++
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
}
``` 
**8.** In findQueueFamilies function, we also add condition to check presentSupport and store that queue's index if it supports present like this;

```c++
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for(const auto& queueFamily : queueFamilies) {


            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
            if(presentSupport) {
                indices.presentFamily = i;
            }

            if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            if(indices.isComplete()) {
                break;
            }

            i++;
        }

        // Assign index to queue families that could be found
        return indices;
    }
```
**9.** While adding logical device, we find and add queue families like this;

```c++
    void createLogicalDevice() {
        // Get queue family indices
        QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
        
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        // Make it set for eliminating duplicates
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                                  indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for(uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }


        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        // Add vector queueCreateInfos here
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;


        createInfo.enabledExtensionCount = 0;

        if(enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if(vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        if(vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }


        // Get pointers of queues
        vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);

    }

```
//////////////////////////////////////////
//// For offscreen rendering use this ////
//// #include <vulkan/vulkan.h> //////////
//////////////////////////////////////////
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <string.h>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>







// ------------------ CONSTANTS ----------------- //
const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;
// ---------------------------------------------- //

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


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

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }

};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                      const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if(func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {

    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if(func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }

}

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


class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* _window;
    VkInstance  _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkSurfaceKHR _surface;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkSwapchainKHR _swapChain;
    std::vector<VkImage> _swapChainImages;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImageView> _swapChainImageViews;
    VkRenderPass _renderPass;

    VkDescriptorSetLayout _descriptorSetLayout;
    VkPipelineLayout _pipelineLayout;

    VkPipeline _graphicsPipeline;
    std::vector<VkFramebuffer> _swapChainFramebuffers;
    VkCommandPool _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    std::vector<VkFence> _imagesInFlight;
    size_t currentFrame = 0;
    bool framebufferResized = false;
    VkBuffer _vertexBuffer;
    VkDeviceMemory _vertexBufferMemory;
    VkBuffer _indexBuffer;
    VkDeviceMemory _indexBufferMemory;

    std::vector<VkBuffer> _uniformBuffers;
    std::vector<VkDeviceMemory> _uniformBuffersMemory;

    VkDescriptorPool _descriptorPool;
    std::vector<VkDescriptorSet> _descriptorSets;

    VkImage _textureImage;
    VkDeviceMemory _textureImageMemory;




    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void *pUserData) {

            /* TODO: CAN BE USED IF NEEDED!
                if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
                    // Message is important enough to show
                }
            */

            std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
    }

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

    bool checkValidationLayerSupport() {

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());


        for(const char* layerName : validationLayers) {
            bool layerFound = false;

            for(const auto& layerProperties : availableLayers) {
                if(strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if(!layerFound) {
                return false;
            }

        }
        return true;
    }

    bool areAllExtensionsIncluded(const char **glfwExtensions, uint32_t& glfwExtensionCount) {

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        int counter = 0;

        for(int i=0; i<glfwExtensionCount; i++) {
            for(int j=0; j<extensions.size(); j++) {
                if(strcmp(glfwExtensions[i],extensions[j].extensionName) == 0)
                    counter++;
            }
        }

        if(counter != glfwExtensionCount)
            return false;

        return true;

    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if(enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;

    }

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

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader model!");
        }

        return shaderModule;
    }

    void createInstance() {

        if(enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("Validation layers requested, but not available!");
        }
        

        VkApplicationInfo appInfo{};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "No Engine";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_0;


        VkInstanceCreateInfo createInfo{};
        createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;


        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();


        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        if (enableValidationLayers) {

            createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames =  validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

        } else {

            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        std::cout << "AreAllExtensionsIncluded: " << areAllExtensionsIncluded(glfwExtensions, glfwExtensionCount) << '\n';


        if(vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }


    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {

        createInfo = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        createInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData       = nullptr;  
    }

    void setupDebugMessenger() {

        if(!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if(CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }                               

    }

    bool isDeviceSuitable(VkPhysicalDevice device) {

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        
        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
               deviceFeatures.geometryShader;


    }

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

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

        if(deviceCount == 0) {
            throw std::runtime_error("failed to find GPU's with Vulkan Support!");
        }


        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

        std::multimap<int, VkPhysicalDevice> candidates;

        for(const auto& device : devices) {
            int score = rateDeviceSuitability(device);
            candidates.insert(std::make_pair(score, device));
        }

        // Check if best candidate is suitable or not
        if(true) {
            _physicalDevice = candidates.rbegin()-> second;
        }
        else {
            throw std::runtime_error("failed to find a suitable GPU");
        }

    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(_physicalDevice);
        

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
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
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pEnabledFeatures = &deviceFeatures;


        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

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

        vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
        vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);

    }

    void createSurface() {
        if(glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

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

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {

        for(const auto& availableFormat : availableFormats) {
            if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
               availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                   return availableFormat;
               }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        
        for(const auto& availablePresentMode : availablePresentModes) {
            if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

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

    void createImageViews() {
        _swapChainImageViews.resize(_swapChainImages.size());

        for(size_t i=0; i<_swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = _swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = _swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if(vkCreateImageView(_device, &createInfo, nullptr, &_swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }

        }
    }

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

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width  = (float) _swapChainExtent.width;
        viewport.height = (float) _swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = _swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports    = &viewport;
        viewportState.scissorCount  = 1;
        viewportState.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth               = 1.0f;
        rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable         = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp          = 0.0f;
        rasterizer.depthBiasSlopeFactor    = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable   = VK_FALSE;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading      = 1.0f;
        multisampling.pSampleMask           = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable      = VK_FALSE;


        // Depth and stencil testing are left here


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


        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable     = VK_FALSE;
        colorBlending.logicOp           = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount   = 1;
        colorBlending.pAttachments      = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_LINE_WIDTH
        };

        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates    = dynamicStates;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount         = 1;
        pipelineLayoutInfo.pSetLayouts            = &_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges    = nullptr;

        if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount          = 2;
        pipelineInfo.pStages             = shaderStages;
        pipelineInfo.pVertexInputState   = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState   = &multisampling;
        pipelineInfo.pDepthStencilState  = nullptr;
        pipelineInfo.pColorBlendState    = &colorBlending;
        pipelineInfo.pDynamicState       = nullptr;
        pipelineInfo.layout              = _pipelineLayout;
        pipelineInfo.renderPass          = _renderPass;
        pipelineInfo.subpass             = 0;
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex   = -1;

        if(vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }


        vkDestroyShaderModule(_device, fragShaderModule, nullptr);
        vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format  = _swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments    = &colorAttachment;
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if(vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

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

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags            = 0;

        if(vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        _commandBuffers.resize(_swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool         = _commandPool;
        allocInfo.level               = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount  = (uint32_t) _commandBuffers.size();

        if(vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for(size_t i=0; i<_commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0;
            beginInfo.pInheritanceInfo = nullptr;

            if(vkBeginCommandBuffer(_commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass  = _renderPass;
            renderPassInfo.framebuffer = _swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0,0};
            renderPassInfo.renderArea.extent = _swapChainExtent;

            VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues    = &clearColor;

            vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

            VkBuffer vertexBuffers[] = {_vertexBuffer};
            VkDeviceSize offsets[]   = {0};
            vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT16);
            vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSets[i], 0, nullptr);
            vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
            vkCmdEndRenderPass(_commandBuffers[i]);

            if(vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer");
            }

        }
    }

    void createSyncObjects() {
        _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        _imagesInFlight.resize(_swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


        for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
            if(vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }

    }

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
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = size;
        bufferInfo.usage       = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if(vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(_device, buffer, bufferMemory, 0);

    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0 ,&data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(_device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexBufferMemory);

        copyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

        vkDestroyBuffer(_device, stagingBuffer, nullptr);
        vkFreeMemory(_device, stagingBufferMemory, nullptr);

    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands()       ;

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        endSingleTimeCommands(commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

        for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void *data;
        vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(_device, stagingBufferMemory);

        createBuffer(bufferSize, 
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);

        copyBuffer(stagingBuffer, _indexBuffer, bufferSize);

        vkDestroyBuffer(_device, stagingBuffer, nullptr);
        vkFreeMemory(_device, stagingBufferMemory, nullptr);
        

    }

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

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        _uniformBuffers.resize(_swapChainImages.size());
        _uniformBuffersMemory.resize(_swapChainImages.size());

        for(size_t i=0; i<_swapChainImages.size(); i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     _uniformBuffers[i],
                                     _uniformBuffersMemory[i]);
        }
    }

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

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                     VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image,
                     VkDeviceMemory& imageMemory) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = static_cast<uint32_t>(width);
        imageInfo.extent.height = static_cast<uint32_t>(height);
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels     = 1;
        imageInfo.arrayLayers   = 1;
        imageInfo.format        = format;
        imageInfo.tiling        = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage         = usage;
        imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.flags         = 0;

        if(vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(_device, image, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if(vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(_device, image, imageMemory, 0);
        
    }

    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = _commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(_graphicsQueue);

        vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);

    }

    void transitionImageLayout(VkImage image, VkFormat format, 
                               VkImageLayout oldLayout,
                               VkImageLayout newLayout) {

        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = 0;


        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage       = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage  = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }


        vkCmdPipelineBarrier(commandBuffer,
                            sourceStage , destinationStage,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);

        endSingleTimeCommands(commandBuffer);                          

    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = {0,0,0};
        region.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer,
                               buffer,
                               image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);

        endSingleTimeCommands(commandBuffer);        

    }



    void createTextureImage() {
        int texWidth, texHeight, texChannels;

        stbi_uc* pixels = stbi_load("../textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if(!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;


        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);


        void *data;
        vkMapMemory(_device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(_device, stagingBufferMemory);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, 
                                         VK_IMAGE_TILING_OPTIMAL,
                                         VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                                         VK_IMAGE_USAGE_SAMPLED_BIT, 
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                                         _textureImage, 
                                         _textureImageMemory);

        transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                                                                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        copyBufferToImage(stagingBuffer, _textureImage, static_cast<uint32_t>(texWidth),
                                                        static_cast<uint32_t>(texHeight));
        transitionImageLayout(_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(_device, stagingBuffer, nullptr);
        vkFreeMemory(_device, stagingBufferMemory, nullptr);

        

    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createTextureImage();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
    }

    void drawFrame() {
        vkWaitForFences(_device, 1, &_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if(_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(_device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }

        _imagesInFlight[imageIndex] = _inFlightFences[currentFrame];

        updateUniformBuffer(imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores    = waitSemaphores;
        submitInfo.pWaitDstStageMask  = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &_commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores    = signalSemaphores;

        vkResetFences(_device, 1, &_inFlightFences[currentFrame]);

        if(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = signalSemaphores;

        VkSwapchainKHR swapChains[] = {_swapChain};
        presentInfo.swapchainCount  = 1;
        presentInfo.pSwapchains     = swapChains;
        presentInfo.pImageIndices   = &imageIndex;
        presentInfo.pResults        = nullptr;

        VkResult result = vkQueuePresentKHR(_presentQueue, &presentInfo);

        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
            return;
        }
        else if(result != VK_SUCCESS) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void mainLoop() {

        while(!glfwWindowShouldClose(_window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(_device);
    }

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

        for(size_t i=0; i< _swapChainImages.size(); i++) {
            vkDestroyBuffer(_device, _uniformBuffers[i], nullptr);
            vkFreeMemory(_device, _uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(_device, _descriptorPool, nullptr);
    }

    void cleanup() {

        cleanupSwapChain();

        vkDestroyImage(_device, _textureImage, nullptr);
        vkFreeMemory(_device, _textureImageMemory, nullptr);

        vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, nullptr);

        vkDestroyBuffer(_device, _indexBuffer, nullptr);
        vkFreeMemory(_device, _indexBufferMemory, nullptr);

        vkDestroyBuffer(_device, _vertexBuffer, nullptr);
        vkFreeMemory(_device, _vertexBufferMemory, nullptr);

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
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch(const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
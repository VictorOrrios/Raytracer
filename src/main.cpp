#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <glm/glm.hpp>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

using namespace std;

// -----------------------------------------------------------------------------
//  Constants & global-scope helpers
// -----------------------------------------------------------------------------
// Initial window size
const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;
// Number of frames in the swapchain
const int MAX_FRAMES_IN_FLIGHT = 2;
// .spirv directory location
const string SPV_DIR = "bin/shaders/";
// Validation layer names to activate if DEBUG flag is defined
const vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};
// Extensions requiered to have
const vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};
// FPS counter variables
double lastTime = glfwGetTime();
int frameCount = 0;
    
// Enable validation layers only when DEBUG is defined.
#ifdef DEBUG
const bool enableValidationLayers = true;
#else
const bool enableValidationLayers = false;
#endif


// TODO Replace with something NOT dummy :p
struct dummy{
    glm::vec2 x;
};
int dummyCount = 10;


struct Camera{
    glm::vec3 location;
    glm::vec3 direction;
};

struct UniformBufferObject
{
    Camera camera;
};





// -----------------------------------------------------------------------------
//  The application class
// -----------------------------------------------------------------------------
class RaytracingApp
{
public:
    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // -------------------------------------------------------------------------
    //  Member variables
    // -------------------------------------------------------------------------

    // GLFW, window, and surfaces
    GLFWwindow* window = nullptr;
    VkSurfaceKHR surface;

    // Instances and meta
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool framebufferResized = false; // If true it resizes the framebuffers

    // Devices
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    // Queues
    VkQueue graphicsAndComputeQueue = VK_NULL_HANDLE;
    VkQueue presentQueue;

    // Swapchains
    VkSwapchainKHR swapChain;
    vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    vector<VkImageView> swapChainImageViews;
    uint32_t currentFrame = 0; // Indicates wich frame is it on the swapchain

    // Pipelines
    VkPipelineLayout pipelineLayout;              
    VkPipeline computePipeline;

    // Commands
    VkCommandPool commandPool;
    vector<VkCommandBuffer> commandBuffers;

    // Descriptors
    VkDescriptorSetLayout descriptorSetLayoutPerFrame;    
    vector<VkDescriptorSet> descriptorSetsPerFrame;
    VkDescriptorSetLayout descriptorSetLayoutGlobal;
    VkDescriptorSet descriptorSetGlobal;
    VkDescriptorPool descriptorPool; 

    // UBOs
    vector<VkBuffer> uniformBuffers;
    vector<VkDeviceMemory> uniformBuffersMemory;
    vector<void *> uniformBuffersMapped;
    
    // Image buffers
    VkImage outputImage;
    VkDeviceMemory outputImageMemory;
    VkImageView outputImageView;

    // SSBOs
    VkBuffer shaderStorageBuffer;
    VkDeviceMemory shaderStorageBufferMemory;

    // Sync Objects
    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> renderFinishedSemaphores;
    vector<VkFence> inFlightFences;


    // Holds the index of the queue family that supports graphics commands
    struct QueueFamilyIndices
    {
        optional<uint32_t> graphicsAndComputeFamily;
        optional<uint32_t> presentFamily;

        bool isComplete() const { return graphicsAndComputeFamily.has_value(); }
    };

    // Swapchain info holder
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        vector<VkSurfaceFormatKHR> formats;
        vector<VkPresentModeKHR> presentModes;
    };    

    // ---------------- Main loop ------------------------------------
    void mainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
            drawFrame();
            if(enableValidationLayers) showFPS();
        }

        vkDeviceWaitIdle(device);
    }

    // ---------------- Cleanup ------------------------------------
    void cleanupSwapChain()
    {

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void cleanup()
    {

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        for (size_t i = 0; i < uniformBuffers.size(); i++)
        {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyImageView(device,outputImageView,nullptr);
        vkDestroyImage(device,outputImage, nullptr);
        vkFreeMemory(device, outputImageMemory, nullptr);

        vkDestroyBuffer(device, shaderStorageBuffer, nullptr);
        vkFreeMemory(device, shaderStorageBufferMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutPerFrame, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutGlobal, nullptr);

        vkDestroyPipeline(device, computePipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(device, commandPool, nullptr);

        vkDestroyDevice(device, nullptr);

        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);

        glfwTerminate();
    }

    // ---------------- GLFW window creation ------------------------------------
    void initWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // We’ll use Vulkan, not OpenGL

        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<RaytracingApp *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    // ---------------- Vulkan initialization pipeline ------------------------------------
    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createDescriptorSetLayout();
        createComputePipeline();
        createCommandPool();
        createUniformBuffers();
        createImageBuffer(WIDTH,HEIGHT);
        createShaderStorageBuffer();
        createDescriptorPool();
        createDescriptorSetsPerFrame();
        createDescriptorSetsGlobal();
        createCommandBuffers();
        createSyncObjects();
    }

    // ---------------- Instance creation ------------------------------------------------
    void createInstance()
    {
        // Verify that requested validation layers are available
        if (enableValidationLayers && !checkValidationLayerSupport())
            throw runtime_error("Validation layers requested, but not available");

        // Application information (optional but helps drivers optimize)
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        // Main instance-creation structure
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        // Add required extensions
        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Add validation layers if enabled
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo; // Hook debug messenger into vkCreateInstance
        }
        else
        {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // Create the Vulkan instance
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw runtime_error("Failed to create Vulkan instance");

        // Optional: print and verify extension support
        checkForExtensionSupport();
    }

    // Return the list of required instance extensions:
    vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (enableValidationLayers)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    // ---------------- Validation layers & debug messengers ------------------------------------------------
    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
            return;

        VkDebugUtilsMessengerCreateInfoEXT ci;
        populateDebugMessengerCreateInfo(ci);

        if (CreateDebugUtilsMessengerEXT(instance, &ci, nullptr, &debugMessenger) != VK_SUCCESS)
            throw runtime_error("Failed to set up debug messenger");
    }

    /**
     * Print all available instance extensions (in DEBUG builds) and ensure
     * that every GLFW-required extension is present.
     */
    void checkForExtensionSupport()
    {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        if(enableValidationLayers){
            cout << "Available Vulkan extensions:\n";
            for (const auto &ext : extensions)
                cout << "\t" << ext.extensionName << '\n';
        }

        uint32_t glfwExtensionCount;
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        for (uint32_t i = 0; i < glfwExtensionCount; ++i)
        {
            bool found = false;
            for (const auto &ext : extensions)
                if (strcmp(ext.extensionName, glfwExtensions[i]) == 0)
                {
                    found = true;
                    break;
                }
            if (!found)
                throw runtime_error(string("GLFW extension not supported: ") + glfwExtensions[i]);
        }

        if(enableValidationLayers)
            cout << "All GLFW-required extensions are supported.\n";
        
    }

    /**
     * Ensure that the VK_LAYER_KHRONOS_validation layer is present
     * when validation is requested.
     */
    bool checkValidationLayerSupport()
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto &layerProps : availableLayers)
                if (strcmp(layerName, layerProps.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            if (!layerFound)
                return false;
        }
        return true;
    }

    /**
     * Populate a VkDebugUtilsMessengerCreateInfoEXT structure with the desired
     * severity & message type filters and the user callback function.
     */
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &ci)
    {
        ci = {};
        ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        ci.pfnUserCallback = debugCallback;
    }

    //  Dynamic-dispatch helpers for the EXT debug-utils extension
    VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
        const VkAllocationCallbacks *pAllocator,
        VkDebugUtilsMessengerEXT *pDebugMessenger)
    {
        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        return func ? func(instance, pCreateInfo, pAllocator, pDebugMessenger)
                    : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DestroyDebugUtilsMessengerEXT(
        VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks *pAllocator)
    {
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

        if (func)
            func(instance, debugMessenger, pAllocator);
    }

    /** Callback that the validation layer calls whenever it has something
     *  important to say. Just print the message to stderr.
     */
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
                  VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                  const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
                  void * /*userData*/)
    {
        cerr << "Validation layer: " << callbackData->pMessage << '\n';
        return VK_FALSE; // Returning VK_FALSE indicates “do not abort”.
    }

    // ---------------- Surface creation ------------------------------
    void createSurface()
    {
        // Creates a window surface linked to the Vulkan instance and GLFW window.
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw runtime_error("failed to create window surface");
        }
    }

    // ---------------- Physical & logical device ------------------------------
    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
            throw runtime_error("No Vulkan-capable GPU found");

        vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        // Rank devices by an application-defined score and pick the best.
        multimap<int, VkPhysicalDevice> candidates;
        for (auto dev : devices)
            candidates.emplace(rateDeviceSuitability(dev), dev);

        if (candidates.rbegin()->first > 0)
            physicalDevice = candidates.rbegin()->second;
        else
            throw runtime_error("Failed to find a suitable GPU");
    }

    // Assign a score to each GPU. Higher is better; 0 means “unsuitable”. 
    int rateDeviceSuitability(VkPhysicalDevice device)
    {
        if (!isDeviceSuitable(device))
            return 0;

        int score = 0;

        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures feats;
        vkGetPhysicalDeviceProperties(device, &props);
        vkGetPhysicalDeviceFeatures(device, &feats);

        // Discrete GPUs are preferred.
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;

        // Maximum 2D image dimension indicates overall performance.
        score += props.limits.maxImageDimension2D;

        return score;
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        VkPhysicalDeviceFeatures feats;
        vkGetPhysicalDeviceFeatures(device, &feats);
        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }
        return feats.geometryShader && findQueueFamilies(device).isComplete() &&
               extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        set<string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    // Search the device’s queue families for one that supports graphics
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, families.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.presentFamily = i;
            }
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (families[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                indices.graphicsAndComputeFamily = i;
                break;
            }
        }
        return indices;
    }

    // Create a logical device and retrieve a graphics queue handle
    void createLogicalDevice()
    {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{}; // Nothing special requested

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
            throw runtime_error("Failed to create logical device");

        // Get the created queues
        vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsAndComputeQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    // ---------------- Swapchain creation ------------------------------------------------
    void createSwapChain()
    {
        // Query support details for swap chains from the current physical device.
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
        
        // Choose the optimal settings from the supported ones.
        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        // Try to request one more than the minimum number of images for better buffering.
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        // Ensure we don't exceed the maximum allowed image count.
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        // Fill out the swap chain creation struct.
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;                             // Always 1 unless using stereoscopic 3D.
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

        // If different queues are used for graphics and presentation, enable sharing.
        if (indices.graphicsAndComputeFamily != indices.presentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // More efficient
            createInfo.queueFamilyIndexCount = 0;                    // Optional
            createInfo.pQueueFamilyIndices = nullptr;                // Optional
        }

        // Apply any required transforms (e.g., rotate screen).
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // No blending with other windows.

        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE; // Don’t render pixels hidden by other windows.

        createInfo.oldSwapchain = VK_NULL_HANDLE; // For recreating the swap chain later.

        // Create the swap chain object.
        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        {
            throw runtime_error("failed to create swap chain");
        }

        // Retrieve the handles of the swap chain images.
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        // Store the format and dimensions for later use.
        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    // Chooses the best surface format (color format and color space) from available options.
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const vector<VkSurfaceFormatKHR> &availableFormats)
    {
        // Prefer a specific format and color space for sRGB color output.
        for (const auto &availableFormat : availableFormats)
        {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM )
            {
                swapChainImageFormat = availableFormat.format;
                return availableFormat;
            }
        }

        throw runtime_error("failed to find correct format");

        // Fallback to the first available format if preferred one isn't found.
        swapChainImageFormat = availableFormats[0].format;
        return availableFormats[0];
    }

    // Chooses the best present mode for displaying images to the screen.
    VkPresentModeKHR chooseSwapPresentMode(const vector<VkPresentModeKHR> &availablePresentModes)
    {
        // Prefer mailbox mode for double buffering and lower latency.
        for (const auto &availablePresentMode : availablePresentModes)
        {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                return availablePresentMode;
            }
        }

        //VK_PRESENT_MODE_MAILBOX_KHR VSync with triple buffering, allways keep latest frame
        //throw runtime_error("failed to find vsync mode");

        // FIFO mode is guaranteed to be available.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // Chooses the resolution of the swap chain images.
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
    {
        // If width/height isn't max uint32_t, it means the extent is already determined by the window system.
        if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max())
        {
            return capabilities.currentExtent;
        }
        else
        {
            int width, height;
            glfwGetFramebufferSize(window, &width, &height);

            // Convert window framebuffer size to Vulkan extent.
            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)};

            // Clamp the values to the supported range by the device.
            actualExtent.width = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    // ---------------- ImageView creation ------------------------------------------------
    // Creates image views for all swap chain images so they can be used as framebuffers.
    void createImageViews()
    {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];

            // 2D images with one mip level and one array layer.
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;

            // Default mapping of color channels.
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            // View the entire image with no mipmapping or multiple layers.
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            // Create the image view and check for success.
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw runtime_error("failed to create image views");
            }
        }
    }

    // ---------------- Compute pipeline creation ------------------------------------------------
    void createComputePipeline()
    {
        // Read compiled shader code from files
        auto computeShaderCode = readFile(SPV_DIR+"raytracer.comp.spv");

        // Create shader modules from code
        VkShaderModule computeShaderModule = createShaderModule(computeShaderCode);

        // ======================
        // SHADER STAGE SETUP
        // ======================

        // Configure the compute shader stage
        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = computeShaderModule;
        computeShaderStageInfo.pName = "main"; // Entry point function

        // ======================
        // PIPELINE LAYOUT
        // ======================

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 2;
        array<VkDescriptorSetLayout,2> descriptorSetLayouts = {descriptorSetLayoutPerFrame,descriptorSetLayoutGlobal};
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw runtime_error("failed to create pipeline layout");
        }

        // ======================
        // PIPELINE CREATION
        // ======================

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.stage = computeShaderStageInfo;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, 
            &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {

            throw runtime_error("failed to create compute pipeline!");
        }

        // Clean up temporary shader modules
        vkDestroyShaderModule(device, computeShaderModule, nullptr);
    }

    // ---------------- Command pool/buffer creation ------------------------------------------------
    void createCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw runtime_error("failed to create command pool");
        }
    }

    void createCommandBuffers()
    {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        {
            throw runtime_error("failed to allocate command buffers");
        }
    }

    // ---------------- Command buffer recording ------------------------------------------------
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional
        beginInfo.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw runtime_error("failed to begin recording command buffer");
        }

            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

            array<VkDescriptorSet,2> descriptorSets= {descriptorSetsPerFrame[currentFrame],descriptorSetGlobal};
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 2, descriptorSets.data(), 0, 0);

            vkCmdDispatch(commandBuffer, (swapChainExtent.width + 15) / 16, (swapChainExtent.height + 15) / 16, 1);

            VkImageMemoryBarrier startBarriers[2]{};
            startBarriers[0] = createMemoryBarrier(outputImage,
                                VK_ACCESS_SHADER_WRITE_BIT,
                                VK_ACCESS_TRANSFER_READ_BIT,
                                VK_IMAGE_LAYOUT_GENERAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
            startBarriers[1] = createMemoryBarrier(swapChainImages[imageIndex],
                                0,
                                VK_ACCESS_TRANSFER_WRITE_BIT,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Ejecutar barreras
            vkCmdPipelineBarrier(commandBuffer,
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0,
                                0, nullptr,
                                0, nullptr,
                                2, startBarriers);

            // 3. Copiar imagen
            VkImageCopy copyRegion{};
            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = {0, 0, 0};
            copyRegion.dstSubresource = copyRegion.srcSubresource;
            copyRegion.dstOffset = {0, 0, 0};
            copyRegion.extent = {swapChainExtent.width, swapChainExtent.height, 1};


            vkCmdCopyImage(commandBuffer,
                        outputImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        swapChainImages[imageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        1, &copyRegion);


            VkImageMemoryBarrier endBarriers[2]{};
            endBarriers[0] = createMemoryBarrier(outputImage,
                            VK_ACCESS_TRANSFER_READ_BIT,
                            VK_ACCESS_SHADER_WRITE_BIT,
                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                            VK_IMAGE_LAYOUT_GENERAL);
            endBarriers[1] = createMemoryBarrier(swapChainImages[imageIndex],
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            0,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

            vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, 
                    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                    0,
                    0, nullptr,
                    0, nullptr,
                    2, endBarriers);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw runtime_error("failed to record command buffer");
        }
    }

    // ---------------- Main draw (dispatch) call ------------------------------------------------
    void drawFrame()
    {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        vkQueueWaitIdle(presentQueue);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            int width = 0, height = 0;
            getWidthHeigth(width,height);
            recreateSwapChain(width,height);
            recreateOutputImage(width,height);
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updateUniformBuffer(currentFrame);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsAndComputeQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            int width = 0, height = 0;
            getWidthHeigth(width,height);
            recreateSwapChain(width,height);
            recreateOutputImage(width,height);
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    

    // ---------------- Frame buffers recreation ------------------------------------
    void recreateSwapChain(int width, int height){
        cleanupSwapChain();

        createSwapChain();
        createImageViews();
    }

    void recreateOutputImage(int width, int height){
        vkDeviceWaitIdle(device);

        vkDestroyImageView(device, outputImageView, nullptr);
        vkDestroyImage(device,outputImage, nullptr);
        vkFreeMemory(device, outputImageMemory, nullptr);

        createImageBuffer(width,height);
        createDescriptorSetsGlobal();
    }


    
    
    
    // ---------------- UBO creation ------------------------------------------------
    void createUniformBuffers()
    {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    // ---------------- Update UBO for animation ------------------------------------------------
    /*
    void updateUniformBuffer(uint32_t currentImage)
    {
        static auto startTime = chrono::high_resolution_clock::now();

        auto currentTime = chrono::high_resolution_clock::now();
        float time = chrono::duration<float, chrono::seconds::period>(currentTime - startTime).count();

        UniformBufferObject ubo{};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
        ubo.proj[1][1] *= -1; // Since we are working with Vulkan and not OpenGL we need to flip

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }
        */
    void updateUniformBuffer(uint32_t currentImage)
    {
        UniformBufferObject ubo{};
        ubo.camera.location = glm::vec3(0.0,0.0,0.0);
        ubo.camera.direction = glm::vec3(0.0,0.0,1.0);

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }


    // ---------------- Image buffer creation ------------------------------------------------
    void createImageBuffer(int width, int height){
        
        createImage(width,height,VK_FORMAT_R8G8B8A8_UNORM,VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT ,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            outputImage,outputImageMemory);

        VkCommandBuffer cmd = beginSingleTimeCommands();
            VkImageMemoryBarrier barrier = createMemoryBarrier(outputImage,
                                            0,
                                            VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_TRANSFER_READ_BIT,
                                            VK_IMAGE_LAYOUT_UNDEFINED,
                                            VK_IMAGE_LAYOUT_GENERAL);
            vkCmdPipelineBarrier(
                cmd,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );
        endSingleTimeCommands(cmd);

        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = outputImage;

        // 2D images with one mip level and one array layer.
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
        // TODO FIX

        // Default mapping of color channels.
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // View the entire image with no mipmapping or multiple layers.
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        // Create the image view and check for success.
        if (vkCreateImageView(device, &createInfo, nullptr, &outputImageView) != VK_SUCCESS)
        {
            throw runtime_error("failed to create image views");
        }

    }

    // ---------------- SSBO creation ------------------------------------------------
    void createShaderStorageBuffer(){
        
        vector<dummy> dummyVec(dummyCount);
        
        VkDeviceSize bufferSize = sizeof(dummy) * dummyCount;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, dummyVec.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffer, shaderStorageBufferMemory);
        // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
        copyBuffer(stagingBuffer, shaderStorageBuffer, bufferSize);

        vkDestroyBuffer(device,stagingBuffer,nullptr);
        vkFreeMemory(device,stagingBufferMemory,nullptr);
    }

    // ---------------- Descriptor layout/pool/set creation ------------------------------------------------
    void createDescriptorSetLayout()
    {
        VkDescriptorSetLayoutBinding layoutBindingA{};

        layoutBindingA.binding = 0;
        layoutBindingA.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindingA.descriptorCount = 1;
        layoutBindingA.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingA.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfoPerFrame{};
        layoutInfoPerFrame.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfoPerFrame.bindingCount = 1;
        layoutInfoPerFrame.pBindings = &layoutBindingA;

        if (vkCreateDescriptorSetLayout(device, &layoutInfoPerFrame, nullptr, &descriptorSetLayoutPerFrame) != VK_SUCCESS)
        {
            throw runtime_error("failed to create descriptor set layout per frame");
        }

        array<VkDescriptorSetLayoutBinding, 2> layoutBindingsB{};

        layoutBindingsB[0].binding = 0;
        layoutBindingsB[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        layoutBindingsB[0].descriptorCount = 1;
        layoutBindingsB[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingsB[0].pImmutableSamplers = nullptr;

        layoutBindingsB[1].binding = 1;
        layoutBindingsB[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBindingsB[1].descriptorCount = 1;
        layoutBindingsB[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingsB[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfoGlobal{};
        layoutInfoGlobal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfoGlobal.bindingCount = 2;
        layoutInfoGlobal.pBindings = layoutBindingsB.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfoGlobal, nullptr, &descriptorSetLayoutGlobal) != VK_SUCCESS)
        {
            throw runtime_error("failed to create descriptor set layout global");
        }
        
    }

    void createDescriptorPool()
    {
        array<VkDescriptorPoolSize, 2> poolSizes{};

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[1].descriptorCount = 1;

        poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[2].descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 3;
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)+2;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw runtime_error("failed to create descriptor pool");
        }
    }

    void createDescriptorSetsPerFrame()
    {
        vector<VkDescriptorSetLayout> layoutsPerFrame(MAX_FRAMES_IN_FLIGHT, descriptorSetLayoutPerFrame);
        VkDescriptorSetAllocateInfo allocInfoPerFrame{};
        allocInfoPerFrame.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfoPerFrame.descriptorPool = descriptorPool;
        allocInfoPerFrame.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfoPerFrame.pSetLayouts = layoutsPerFrame.data();

        descriptorSetsPerFrame.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &allocInfoPerFrame, descriptorSetsPerFrame.data()) != VK_SUCCESS)
        {
            throw runtime_error("failed to allocate descriptor sets per frame");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            VkDescriptorBufferInfo uniformBufferInfo{};
            uniformBufferInfo.buffer = uniformBuffers[i];
            uniformBufferInfo.offset = 0;
            uniformBufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = descriptorSetsPerFrame[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &uniformBufferInfo;
            descriptorWrite.pImageInfo = nullptr;       // Optional
            descriptorWrite.pTexelBufferView = nullptr; // Optional

            vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
        }

        vector<VkDescriptorSetLayout> layoutsGlobal(1, descriptorSetLayoutGlobal);
        VkDescriptorSetAllocateInfo allocInfoGlobal{};
        allocInfoGlobal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfoGlobal.descriptorPool = descriptorPool;
        allocInfoGlobal.descriptorSetCount = 1;
        allocInfoGlobal.pSetLayouts = layoutsGlobal.data();

        if (vkAllocateDescriptorSets(device, &allocInfoGlobal, &descriptorSetGlobal) != VK_SUCCESS)
        {
            throw runtime_error("failed to allocate descriptor sets global");
        }
    }

    void createDescriptorSetsGlobal(){

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = outputImageView;
        imageInfo.sampler = VK_NULL_HANDLE;

        VkDescriptorBufferInfo storageBufferInfo{};
        storageBufferInfo.buffer = shaderStorageBuffer;
        storageBufferInfo.offset = 0;
        storageBufferInfo.range = sizeof(dummy) * dummyCount;

        array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSetGlobal;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSetGlobal;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &storageBufferInfo;

        vkUpdateDescriptorSets(device, 2, descriptorWrites.data(), 0, nullptr);
    }

    // ---------------- Sync object creation ------------------------------------------------
    void createSyncObjects()
    {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
        {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {

                throw runtime_error("failed to create synchronization objects for a frame");
            }
        }

    }

    // ---------------- Helper functions ------------------------------------------------

    // File reader to load .spv files (shaders)
    static vector<char> readFile(const string &filename)
    {
        ifstream file(filename, ios::ate | ios::binary);

        if (!file.is_open())
        {
            throw runtime_error("failed to open file: "+filename);
        }

        size_t fileSize = (size_t)file.tellg();
        vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    // Gets the width and height of the window
    void getWidthHeigth(int& width, int& height){
        glfwGetFramebufferSize(window, &width, &height);
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);
    }

    // Bytecode to shaderModule
    VkShaderModule createShaderModule(const vector<char> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw runtime_error("failed to create shader module");
        }

        return shaderModule;
    }

    // Appends to commandPool a single time operation to copy from a source buffer to a destination buffer
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        endSingleTimeCommands(commandBuffer);
    }

    // Used with endSingleTimeCommands to append a command buffer to the commandPool
    VkCommandBuffer beginSingleTimeCommands() {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        return commandBuffer;
    }

    // Used with beginSingleTimeCommands to append a command buffer to the commandPool
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(graphicsAndComputeQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsAndComputeQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    // Creates a simple memory barrier to change access bit masks or layouts of images
    VkImageMemoryBarrier createMemoryBarrier(
        VkImage image,
        VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ){
        VkImageMemoryBarrier barrier;

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

        return barrier;
    }

    // Creates a simple image and allocates the memory for it
    void createImage(uint32_t width, uint32_t height, VkFormat format, 
        VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
        VkImage& image, VkDeviceMemory& imageMemory) {

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw runtime_error("failed to create image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw runtime_error("failed to allocate image memory");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    // Creates a VkBuffer and allocates the memory for it
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
    {

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw runtime_error("failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate buffer memory");
        }

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }

    

    // Queries the given physical device to check its support for swap chains.
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
    {
        SwapChainSupportDetails details;

        // Get surface capabilities like min/max number of images, min/max dimensions, etc.
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        // If formats are supported, fetch them.
        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        // If present modes are supported, fetch them.
        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    // Finds if all the type filters pass on a set of property flags
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw runtime_error("failed to find suitable memory type");
    }

    // Puts on screen the current fps
    void showFPS() {
        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        frameCount++;
        
        if (delta >= 1.0) {
            double fps = frameCount / delta;
            //cout << "FPS: " << fps << endl;
            
            string title = "Vulkan App - FPS: " + to_string((int)fps);
            glfwSetWindowTitle(window, title.c_str());
            
            frameCount = 0;
            lastTime = currentTime;
        }
    }


};

int main()
{
    RaytracingApp app;
    try
    {
        app.run();
    }
    catch (const exception &e)
    {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

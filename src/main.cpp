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
#include <glm/gtx/euler_angles.hpp>
#include <chrono>


#include "scene.hpp"

using namespace std;

// Enable validation layers only when DEBUG is defined.
#ifdef DEBUG
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif


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

// Number of shader storage buffers used
const int numSSBO = 4;

// World vetors
const glm::vec4 worldFront = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
const glm::vec4 worldUp    = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

// Camera control constants
const float moveSpeed = 1.0;
const float rollSpeed = 80.0;
const float shiftMult = 2.5;
const float fovIncreaseAmount = 1.0;
const float sensitivityInitial = 0.1;

// Camera view parameters
const float fovInitial = 50.0f; 
const float nearClip = 0.1f; 
const float farClip = 100.0f; 

// Initial parameters for the camera
const glm::vec3 cameraPositionInitial = glm::vec3(0.0f, 0.0f, -3.0f);
const glm::vec3 cameraFrontInitial = glm::vec3(0.0f, 0.0f, -4.0f);
const glm::vec3 cameraUpInitial = glm::vec3(worldUp);
const glm::vec3 frontVectorTemp = glm::normalize(cameraFrontInitial-cameraPositionInitial);
const float yawInitial = glm::degrees(glm::atan(frontVectorTemp.x,-frontVectorTemp.z));
const float pitchInitial = glm::degrees(glm::asin(frontVectorTemp.y));
const float rollInitial = 0.0f;

// If true disables keyboard movement and aplies constant forward velocity
// Turn the controls from 6 degrees of movement to the controls of a plane
const bool planeMode = false;

// Initial value for the toggle that indicates if the frame accumulation must be on
const bool frameAccumulationInitial = true;

// Static render mode, if true only renders the first frame of the scene 
const bool staticRenderMode = true;


// -----------------------------------------------------------------------------
//  The application class
// -----------------------------------------------------------------------------
class RaytracingApp
{
public:
    // Initializes and runs the raytracing window
    void run()
    {
        initWindow();
        initVulkan();
        if(staticRenderMode){
            mainLoopStatic();
        }else{
            mainLoop();
        }
        cleanup();
    }

private:

    // -------------------------------------------------------------------------
    //  Struct definitions
    // -------------------------------------------------------------------------
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

    // All the info about the camera in each frame
    struct Camera
    {
        glm::mat4 view;
        glm::mat4 viewInv;
        glm::mat4 proj;
        glm::mat4 projInv;
        glm::mat4 viewproj;
        glm::vec3 position;
        float tanHalfFOV;
    };

    // Info to pass with the uniform buffer each frame
    struct UniformBufferObject
    {
        Camera camera;
    };

    struct PushConstants
    {
        float time;
        int frameCount;
        int total_lights;
        float lights_strength_sum;
        glm::vec3 world_up;
        bool reset_frame_accumulation;
    };

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
    VkDescriptorSetLayout descriptorSetLayoutFrameAccum;
    VkDescriptorSet descriptorSetFrameAccum;
    VkDescriptorPool descriptorPool; 

    // Push constants
    PushConstants pushConstants;
    

    // UBOs
    vector<VkBuffer> uniformBuffers;
    vector<VkDeviceMemory> uniformBuffersMemory;
    vector<void *> uniformBuffersMapped;
    
    // Image buffers
    VkImage outputImage;
    VkDeviceMemory outputImageMemory;
    VkImageView outputImageView;

    // SSBOs
    vector<VkBuffer> shaderStorageBuffers = vector<VkBuffer>(numSSBO);
    vector<VkDeviceMemory> shaderStorageBufferMemory = vector<VkDeviceMemory>(numSSBO);

    // Frame accumulation buffers
    VkBuffer colorAccumulationBuffer;
    VkDeviceMemory colorAccumulationBufferMemory;
    VkBuffer sampleCountBuffer;
    VkDeviceMemory sampleCountBufferMemory;

    // Sync Objects
    vector<VkSemaphore> imageAvailableSemaphores;
    vector<VkSemaphore> renderFinishedSemaphores;
    vector<VkFence> inFlightFences;

    // Scene where all the objects reside
    Scene scene;

    // FPS counter variables
    float lastFrame;
    int frameCount = 0;

    // Mouse controls variables
    bool mouseCaptured = true;
    bool firstMouse = true;

    // Camera variables
    float fov = fovInitial;
    glm::vec3 cameraPos = cameraPositionInitial;
    glm::vec3 cameraFront = cameraFrontInitial;
    glm::vec3 cameraUp = cameraUpInitial;
    float yaw = yawInitial;
    float pitch = pitchInitial;
    float roll = rollInitial;
    glm::mat4 rotation = glm::yawPitchRoll(
                            glm::radians(yaw),
                            glm::radians(pitch),
                            glm::radians(roll)
                        );

    // Frame accumulation
    bool resetFrameAccumulation = true;
    bool frameAccumulationOn = frameAccumulationInitial;

    
    // ---------------- Main loops ------------------------------------
    void mainLoop()
    {
        float deltaTime;
        while (!glfwWindowShouldClose(window))
        {
            float currentFrame = glfwGetTime();
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            glfwPollEvents();
            processInput(window, deltaTime);
            drawFrame();
            showFPS();
        }

        vkDeviceWaitIdle(device);
    }

    void mainLoopStatic()
    {
        drawFrame();

        while (!glfwWindowShouldClose(window))
            glfwPollEvents();

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

        for (size_t i = 0; i < shaderStorageBuffers.size(); i++)
        {
            vkDestroyBuffer(device, shaderStorageBuffers[i], nullptr);
            vkFreeMemory(device, shaderStorageBufferMemory[i], nullptr);
        }

        vkDestroyBuffer(device, colorAccumulationBuffer, nullptr);
        vkFreeMemory(device, colorAccumulationBufferMemory, nullptr);

        vkDestroyBuffer(device, sampleCountBuffer, nullptr);
        vkFreeMemory(device, sampleCountBufferMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutPerFrame, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutGlobal, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayoutFrameAccum, nullptr);

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
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        if(!staticRenderMode){
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(window, mouse_callback_static);
            glfwSetMouseButtonCallback(window, focus_callback_static);
            glfwSetScrollCallback(window, mouse_scroll_callback_static);
        }
    }

    // ---------------- GLFW input handling ------------------------------------
    // Keyboard controller
    void processInput(GLFWwindow* window, float deltaTime) {
        if(!mouseCaptured) return;

        float moveSpeedDelta = moveSpeed * deltaTime;
        float rollSpeedDelta = rollSpeed * deltaTime;
        
        bool rotateMatrix = false;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
            moveSpeedDelta *= shiftMult;
            rollSpeedDelta *= shiftMult;
        }

        if(planeMode){
            cameraPos += moveSpeedDelta * cameraFront;
        }else{
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
                cameraPos += moveSpeedDelta * glm::normalize(cameraFront);
                resetFrameAccumulation = true;
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
                cameraPos -= moveSpeedDelta * glm::normalize(cameraFront);
                resetFrameAccumulation = true;
            }
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
                cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * moveSpeedDelta;
                resetFrameAccumulation = true;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
                cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * moveSpeedDelta;
                resetFrameAccumulation = true;
            }
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
                cameraPos += moveSpeedDelta * cameraUp;
                resetFrameAccumulation = true;
            }
            if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS){
                cameraPos -= moveSpeedDelta * cameraUp;
                resetFrameAccumulation = true;
            }
        }


        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
            glfwSetWindowShouldClose(window,true);
        
        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
            roll -= rollSpeedDelta;
            rotateMatrix = true;
        }
        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
            roll += rollSpeedDelta;
            rotateMatrix = true;
        }
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS){
            roll = rollInitial;
            rotateMatrix = true;
        }   
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
            roll = rollInitial;
            pitch = pitchInitial;
            yaw = yawInitial;
            cameraPos = cameraPositionInitial;
            fov = fovInitial;
            rotateMatrix = true;
            frameAccumulationOn = frameAccumulationInitial;
        }

        static bool xBounce = false;
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS && !xBounce){
            frameAccumulationOn = !frameAccumulationOn;
            xBounce = true;
        } 
        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_RELEASE && xBounce){
            xBounce = false;
        } 

        if(rotateMatrix){
            if(roll>360.0) roll -= 360.0;
            if(roll<0.0) roll += 360.0;
            updateCamera();
        }

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS){
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseCaptured = false;
        }
    }

    // Window focus controller
    void focus_callback(int button, int action, int mods){
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && !mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // capturar cursor
            mouseCaptured = true;
            firstMouse = true;
        }
    }

    // Mouse controller
    void mouse_callback(double xpos, double ypos) {
        
        static float lastX;
        static float lastY;

        if(!mouseCaptured) return;

        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos;
        lastX = xpos;
        lastY = ypos;

        float sensitivity = sensitivityInitial * fov/fovInitial;
        xoffset *= sensitivity;
        yoffset *= sensitivity;

        float rollRad = -glm::radians(roll);
        yaw   += xoffset * cos(rollRad) - yoffset * sin(rollRad);
        pitch += xoffset * sin(rollRad) + yoffset * cos(rollRad);

        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;

        if(yaw>360.0) yaw -= 360.0;
        if(yaw<0.0) yaw += 360.0;

        updateCamera();
    }

    void mouse_scroll_callback(double xoffset, double yoffset) {
        fov -= static_cast<float>(yoffset)*fovIncreaseAmount;
        if(fov < 1.0) fov = 1.0;
        if(fov > 160.0) fov = 160.0;
        resetFrameAccumulation = true;
    }


    // Updates rotation matrix and camera front and up
    void updateCamera(){
        resetFrameAccumulation = true;

        rotation = glm::yawPitchRoll(
            -glm::radians(yaw),
            glm::radians(pitch),
            -glm::radians(roll)
        );

        cameraFront = glm::normalize(glm::vec3(rotation * worldFront));
        cameraUp = glm::normalize(glm::vec3(rotation * worldUp));
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
        createShaderStorageBuffers();
        createFrameAccumulationBuffers(WIDTH,HEIGHT);
        createDescriptorPool();
        createDescriptorSets(WIDTH,HEIGHT);
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
            if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR)
            {
                return availablePresentMode;
            }
        }

        //VK_PRESENT_MODE_MAILBOX_KHR VSync with triple buffering, allways keep latest frame
        //VK_PRESENT_MODE_FIFO_KHR Double buffering if queue is full it waits
        //VK_PRESENT_MODE_IMMEDIATE_KHR Allways fresher frame, no throtle
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
        pipelineLayoutInfo.setLayoutCount = 3;
        array<VkDescriptorSetLayout,3> descriptorSetLayouts = {descriptorSetLayoutPerFrame,descriptorSetLayoutGlobal,descriptorSetLayoutFrameAccum};
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

        // Push constants set-up
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

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

            array<VkDescriptorSet,3> descriptorSets= {descriptorSetsPerFrame[currentFrame],descriptorSetGlobal, descriptorSetFrameAccum};
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 3, descriptorSets.data(), 0, 0);

            vkCmdPushConstants(commandBuffer,pipelineLayout,VK_SHADER_STAGE_COMPUTE_BIT,0,sizeof(PushConstants),&pushConstants);

            vkCmdDispatch(commandBuffer, (swapChainExtent.width + 31) / 32, (swapChainExtent.height + 31) / 32, 1);

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
            recreateFrameAccumulationBuffers(width,height);
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        updatePushConstantsPre();

        updateUniformBuffer(currentFrame);

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        updatePushConstantsPost();

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
            recreateFrameAccumulationBuffers(width,height);
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
        vkDestroyImageView(device, outputImageView, nullptr);
        vkDestroyImage(device,outputImage, nullptr);
        vkFreeMemory(device, outputImageMemory, nullptr);

        createImageBuffer(width,height);
        createDescriptorSetsGlobal();
    }

    void recreateFrameAccumulationBuffers(int width, int height){
        vkDestroyBuffer(device, colorAccumulationBuffer, nullptr);
        vkFreeMemory(device, colorAccumulationBufferMemory, nullptr);
        vkDestroyBuffer(device, sampleCountBuffer, nullptr);
        vkFreeMemory(device, sampleCountBufferMemory, nullptr);

        createFrameAccumulationBuffers(width,height);
        createDescriptorSetsFrameAccumulation(width,height);
    }


    // ---------------- Push constants update per each frame ------------------------------------------------
    void updatePushConstantsPre(){
        pushConstants.time = lastFrame;
        pushConstants.frameCount = frameCount;
        pushConstants.total_lights = scene.lightsVec.size();
        pushConstants.lights_strength_sum = scene.lights_strength_sum;
        pushConstants.world_up = worldUp;
        if(frameAccumulationOn){
            pushConstants.reset_frame_accumulation = resetFrameAccumulation;
        }else{
            pushConstants.reset_frame_accumulation = true;
        }
    }

    void updatePushConstantsPost(){
        resetFrameAccumulation = false;
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


    // ---------------- UBO update per each frame ------------------------------------------------
    void updateUniformBuffer(uint32_t currentImage)
    {
        UniformBufferObject ubo{};
        
        ubo.camera.view = glm::lookAt(
            cameraPos,    
            cameraPos + cameraFront,
            cameraUp     
        );

        ubo.camera.viewInv = glm::inverse(ubo.camera.view); 
        
        float aspectRatio = swapChainExtent.width/swapChainExtent.height;
        
        ubo.camera.proj = glm::perspective(
            glm::radians(fov), 
            aspectRatio,
            nearClip,
            farClip
        );
        //ubo.camera.proj[1][1] *= -1.0f;

        ubo.camera.projInv = glm::inverse(ubo.camera.proj);

        ubo.camera.viewproj = ubo.camera.view * ubo.camera.proj;

        ubo.camera.position = cameraPos;

        ubo.camera.tanHalfFOV = tan(glm::radians(fov) / 2.0);

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
    void createShaderStorageBuffers(){
        createSSBOVector(0,scene.sphereVec);
        createSSBOVector(1,scene.materialVec);
        createSSBOVector(2,scene.lightsVec);
        createSSBOVector(3,scene.triangleVec);
    }

    template <typename T>
    void createSSBOVector(int index, vector<T> dataVector){

        if (shaderStorageBuffers[index] != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, shaderStorageBuffers[index], nullptr);
            vkFreeMemory(device, shaderStorageBufferMemory[index], nullptr);
        }

        VkDeviceSize bufferSize = sizeof(T) * dataVector.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
            stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, dataVector.data(), (size_t)bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[index], shaderStorageBufferMemory[index]);
        // Copy data from the staging buffer (host) to the shader storage buffer (GPU)
        copyBuffer(stagingBuffer, shaderStorageBuffers[index], bufferSize);

        vkDestroyBuffer(device,stagingBuffer,nullptr);
        vkFreeMemory(device,stagingBufferMemory,nullptr);
    }

    // ---------------- Frame accumulation buffers creation ------------------------------------------------
    void createFrameAccumulationBuffers(int width, int height){
        VkDeviceSize imageSizeColor = width * height * sizeof(glm::vec4);
        createBuffer(imageSizeColor, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorAccumulationBuffer, colorAccumulationBufferMemory);
        initializeBufferWithZeros(colorAccumulationBuffer,imageSizeColor);
        
        VkDeviceSize imageSize = width * height * sizeof(uint32_t);
        createBuffer(imageSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sampleCountBuffer, sampleCountBufferMemory);        
        initializeBufferWithZeros(sampleCountBuffer,imageSize);
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

        array<VkDescriptorSetLayoutBinding, numSSBO+1> layoutBindingsB{};

        layoutBindingsB[0].binding = 0;
        layoutBindingsB[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        layoutBindingsB[0].descriptorCount = 1;
        layoutBindingsB[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        layoutBindingsB[0].pImmutableSamplers = nullptr;

        for(int i = 1; i < 1+numSSBO; i++){
            layoutBindingsB[i].binding = static_cast<uint32_t>(i);
            layoutBindingsB[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBindingsB[i].descriptorCount = 1;
            layoutBindingsB[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindingsB[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfoGlobal{};
        layoutInfoGlobal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfoGlobal.bindingCount = static_cast<uint32_t>(layoutBindingsB.size());
        layoutInfoGlobal.pBindings = layoutBindingsB.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfoGlobal, nullptr, &descriptorSetLayoutGlobal) != VK_SUCCESS)
        {
            throw runtime_error("failed to create descriptor set layout global");
        }

        array<VkDescriptorSetLayoutBinding, 2> layoutBindingsC{};

        for(int i = 0; i<2; i++){
            layoutBindingsC[i].binding = i;
            layoutBindingsC[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBindingsC[i].descriptorCount = 1;
            layoutBindingsC[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            layoutBindingsC[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfoFrameAccumulation{};
        layoutInfoFrameAccumulation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfoFrameAccumulation.bindingCount = static_cast<uint32_t>(layoutBindingsC.size());
        layoutInfoFrameAccumulation.pBindings = layoutBindingsC.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfoFrameAccumulation, nullptr, &descriptorSetLayoutFrameAccum) != VK_SUCCESS)
        {
            throw runtime_error("failed to create descriptor set layout frame accumulation");
        }
        
    }

    void createDescriptorPool()
    {
        array<VkDescriptorPoolSize, 1 + 1+numSSBO + 2> poolSizes{};

        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        poolSizes[1].descriptorCount = 1;

        for(int i = 2; i < poolSizes.size(); i++){
            poolSizes[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSizes[i].descriptorCount = 1;
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = poolSizes.size();
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = poolSizes.size()+static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)-1;

        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw runtime_error("failed to create descriptor pool");
        }
    }

    void createDescriptorSets(int width, int height){
        // Per frame
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

        createDescriptorSetsPerFrame();

        // Global
        VkDescriptorSetAllocateInfo allocInfoGlobal{};
        allocInfoGlobal.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfoGlobal.descriptorPool = descriptorPool;
        allocInfoGlobal.descriptorSetCount = 1;
        allocInfoGlobal.pSetLayouts = &descriptorSetLayoutGlobal;

        if (vkAllocateDescriptorSets(device, &allocInfoGlobal, &descriptorSetGlobal) != VK_SUCCESS)
        {
            throw runtime_error("failed to allocate descriptor sets global");
        }

        createDescriptorSetsGlobal();
        
        // Frame accumulation
        VkDescriptorSetAllocateInfo allocInfoFrameAccum{};
        allocInfoFrameAccum.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfoFrameAccum.descriptorPool = descriptorPool;
        allocInfoFrameAccum.descriptorSetCount = 1;
        allocInfoFrameAccum.pSetLayouts = &descriptorSetLayoutFrameAccum;

        if (vkAllocateDescriptorSets(device, &allocInfoFrameAccum, &descriptorSetFrameAccum) != VK_SUCCESS)
        {
            throw runtime_error("failed to allocate descriptor sets for frame accumulation");
        }

        createDescriptorSetsFrameAccumulation(width,height);
    }

    void createDescriptorSetsPerFrame(){
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
    }

    void createDescriptorSetsGlobal(){
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = outputImageView;
        imageInfo.sampler = VK_NULL_HANDLE;

        vector<VkDescriptorBufferInfo> ssboInfos = vector<VkDescriptorBufferInfo>(numSSBO);

        // Spheres SSBO
        ssboInfos[0].buffer = shaderStorageBuffers[0];
        ssboInfos[0].offset = 0;
        ssboInfos[0].range = sizeof(Sphere) * scene.sphereVec.size();

        // Materials SSBO
        ssboInfos[1].buffer = shaderStorageBuffers[1];
        ssboInfos[1].offset = 0;
        ssboInfos[1].range = sizeof(Material) * scene.materialVec.size();

        // Lights SSBO
        ssboInfos[2].buffer = shaderStorageBuffers[2];
        ssboInfos[2].offset = 0;
        ssboInfos[2].range = sizeof(Light) * scene.lightsVec.size();

        // Triangles SSBO
        ssboInfos[3].buffer = shaderStorageBuffers[3];
        ssboInfos[3].offset = 0;
        ssboInfos[3].range = sizeof(Triangle) * scene.triangleVec.size();

        array<VkWriteDescriptorSet, 1+numSSBO> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSetGlobal;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        for(int i = 1; i < 1+numSSBO; i++){
            descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[i].dstSet = descriptorSetGlobal;
            descriptorWrites[i].dstBinding = i;
            descriptorWrites[i].dstArrayElement = 0;
            descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[i].descriptorCount = 1;
            descriptorWrites[i].pBufferInfo = &ssboInfos[i-1];
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void createDescriptorSetsFrameAccumulation(int width, int height){
        vector<VkDescriptorBufferInfo> ssboInfos(2);

        // Spheres SSBO
        ssboInfos[0].buffer = colorAccumulationBuffer;
        ssboInfos[0].offset = 0;
        ssboInfos[0].range = width * height * sizeof(glm::vec4);

        // Materials SSBO
        ssboInfos[1].buffer = sampleCountBuffer;
        ssboInfos[1].offset = 0;
        ssboInfos[1].range = width * height * sizeof(uint32_t);


        array<VkWriteDescriptorSet, 2> descriptorWrites{};
        for(int i = 0; i < descriptorWrites.size(); i++){
            descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[i].dstSet = descriptorSetFrameAccum;
            descriptorWrites[i].dstBinding = i;
            descriptorWrites[i].dstArrayElement = 0;
            descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[i].descriptorCount = 1;
            descriptorWrites[i].pBufferInfo = &ssboInfos[i];
        }

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
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

    // Fills the buffer with zeros up to size
    void initializeBufferWithZeros(VkBuffer buffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        vkCmdFillBuffer(commandBuffer, buffer, 0, size, 0);

        VkBufferMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            .buffer = buffer,
            .offset = 0,
            .size = size
        };
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 0, nullptr, 1, &barrier, 0, nullptr
        );

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
        static double lastTime = glfwGetTime();
        double currentTime = glfwGetTime();
        double delta = currentTime - lastTime;
        frameCount++;
        
        if (delta >= 1.0) {
            double fps = frameCount / delta;
            //cout << "FPS: " << fps << endl;
            
            if(enableValidationLayers){
                string title = "Vulkan App - FPS: " + to_string((int)fps);
                glfwSetWindowTitle(window, title.c_str());
            }
            
            frameCount = 0;
            lastTime = currentTime;
        }
    }

    // ---------------- Static callbacks ------------------------------------------------

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
    {
        auto app = reinterpret_cast<RaytracingApp *>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    static void focus_callback_static(GLFWwindow* window, int button, int action, int mods){
        RaytracingApp* app = static_cast<RaytracingApp*>(glfwGetWindowUserPointer(window));
        app->focus_callback(button,action,mods);
    }

    static void mouse_callback_static(GLFWwindow* window, double xpos, double ypos){
        RaytracingApp* app = static_cast<RaytracingApp*>(glfwGetWindowUserPointer(window));
        app->mouse_callback(xpos,ypos);
    }

    static void mouse_scroll_callback_static(GLFWwindow* window, double xoffset, double yoffset) {
        RaytracingApp* app = static_cast<RaytracingApp*>(glfwGetWindowUserPointer(window));
        app->mouse_scroll_callback(xoffset,yoffset);
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

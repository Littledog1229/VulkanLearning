#include "HelloTriangle.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <set>

#include "spdlog/spdlog.h"

// Day 1 (1/17/2024)
// Two and a half hours, 264-284 lines in, and im only halfway done with setup...
// I still have Presentation, Graphics Pipeline Basics, and Drawing AS MAIN TABS left
//  . I literally haven't even selected a GPU to do anything with yet!
//
// For reference on how much different (and harder) this is to OpenGL, here is a link to the HelloTriangle for OpenGL
// . https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/2.2.hello_triangle_indexed/hello_triangle_indexed.cpp

// Day 2 [Part 1] (1/18/2024)
//  . Start Time:  9:32   AM
//  . End   Time: 10:29   AM
//  . Line Count: 415-436 Lines
//
// It is now 10:13 AM and I am finished with the Setup part!
//  . Only 12-13 more sections to go until I can see the triangle!!!!! (I have done 5 so far)
//
// The time has advanced to 10:29 AM and I have to prepare and leave for class.
// . I have completed another section, getting the presentation queue

namespace {
    GLFWwindow* window = nullptr;

    uint32_t width  = 1080;
    uint32_t height = 720;

    // Vulkan
    VkInstance               vk_instance;
    VkDebugUtilsMessengerEXT vk_debug_messenger;
    VkSurfaceKHR             vk_surface;
    VkPhysicalDevice         vk_physical_device = VK_NULL_HANDLE; // Implicitly Destroyed
    VkDevice                 vk_logical_device;
    VkQueue                  vk_graphics_queue;
    VkQueue                  vk_presentation_queue;

    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;

#endif
}

struct QueueFamilyIndices {
    std::optional<uint32_t> GraphicsFamilyQueue;
    std::optional<uint32_t> PresentationFamilyQueue;

    [[nodiscard]] bool isComplete() const { return GraphicsFamilyQueue.has_value() && PresentationFamilyQueue.has_value(); }
};

void initWindow();
void initVulkan();
void mainLoop();
void cleanup();

uint32_t helloTriangle() {
    try {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    } catch (const std::exception& e) {
        spdlog::error(" . Problem: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Vulkan Extension Creators
VkResult createDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks*              allocator,
    VkDebugUtilsMessengerEXT*                 debug_messenger
);

// Vulkan Extension Destructors
void destroyDebugUtilsMessengerEXT(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     debug_messenger,
    const VkAllocationCallbacks* allocator
);

// Vulkan Objects
bool                     checkValidationLayerSupport();
std::vector<const char*> getRequiredExtensions();

// Vulkan Debug Callback
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);

// Other Vulkan Stuff
void               populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
bool               isDeviceSuitable(VkPhysicalDevice);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

// Vulkan Creation
void createInstance();
void setupDebugMessenger();
void createSurface();
void selectPhysicalDevice();
void createLogicalDevice();

// Method Implementations
void initWindow() {
    // Lets just assume everything works with GLFW for now...

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Hello Triangle - Vulkan", nullptr, nullptr);
}
void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
}
void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
}
void cleanup() {
    if (enable_validation_layers) {
        destroyDebugUtilsMessengerEXT(vk_instance, vk_debug_messenger, nullptr);
    }

    vkDestroyDevice(vk_logical_device, nullptr);
    vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
    vkDestroyInstance(vk_instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}

// Vulkan stuff
void createInstance() {
    if (enable_validation_layers && !checkValidationLayerSupport())
        throw std::runtime_error{"Some requested validation layers were not available!"};

    VkApplicationInfo vk_app_info{};

    vk_app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vk_app_info.pApplicationName   = "Hello Triangle";
    vk_app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    vk_app_info.pEngineName        = "No Engine";
    vk_app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    vk_app_info.apiVersion         = VK_VERSION_1_0;

    uint32_t available_extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions{available_extension_count};
    vkEnumerateInstanceExtensionProperties(nullptr, &available_extension_count, available_extensions.data());

    spdlog::info("Available Extensions:");

    for (auto [extensionName, specVersion] : available_extensions)
        spdlog::info(" . {}: {}", extensionName, specVersion);

    uint32_t     layer_count = 0;
    const char** layers      = nullptr;

    if (enable_validation_layers) {
        layer_count = static_cast<uint32_t>(validation_layers.size());;
        layers = validation_layers.data();
    }

    auto extensions = getRequiredExtensions();

    VkInstanceCreateInfo vk_create_info{};

    vk_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_create_info.pApplicationInfo        = &vk_app_info;
    vk_create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    vk_create_info.ppEnabledExtensionNames = extensions.data();
    vk_create_info.enabledLayerCount       = layer_count;
    vk_create_info.ppEnabledLayerNames     = layers;

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (enable_validation_layers) {
        populateDebugMessengerCreateInfo(debug_create_info);
        vk_create_info.pNext = &debug_create_info;
    }

    if (const VkResult create_result = vkCreateInstance(&vk_create_info, nullptr, &vk_instance); create_result != VK_SUCCESS)
        throw std::runtime_error{"Failed to create Vulkan Instance!"};
}

bool checkValidationLayerSupport() {
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> layer_properties{layer_count};
    vkEnumerateInstanceLayerProperties(&layer_count, layer_properties.data());

    for (auto* layer_name : validation_layers) {
        bool layer_found = false;

        for (const auto& [layerName, specVersion, implementationVersion, description] : layer_properties) {
            if (strcmp(layerName, layer_name) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
            return false;
    }

    return true;
}

std::vector<const char*> getRequiredExtensions() {
    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions      = nullptr;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions{glfw_extensions, glfw_extensions + glfw_extension_count};

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

void setupDebugMessenger() {
    if (!enable_validation_layers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populateDebugMessengerCreateInfo(create_info);

    if (createDebugUtilsMessengerEXT(vk_instance, &create_info, nullptr, &vk_debug_messenger) != VK_SUCCESS)
        throw std::runtime_error{"Failed to setup debug messenger!"};
}

void selectPhysicalDevice() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(vk_instance, &device_count, nullptr);

    if (device_count == 0)
        throw std::runtime_error{"Failed to find a GPU with Vulkan support."};

    std::vector<VkPhysicalDevice> devices{device_count};
    vkEnumeratePhysicalDevices(vk_instance, &device_count, devices.data());

    for (const auto device : devices) {
        if (isDeviceSuitable(device)) {
            vk_physical_device = device;
            break;
        }
    }

    if (vk_physical_device == VK_NULL_HANDLE)
        throw std::runtime_error{"Failed to find a GPU with Suitable Vulkan support."};
}

// There other approaches to picking the device rather than "is this device suitable", such as:
// . Using a multimap and giving the available devices a "score" and choosing the highest scoring device.
// (which can be seen at https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families)

// A bunch of fancy checks can be done in here to make sure this device is capable of doing what I want
bool isDeviceSuitable(const VkPhysicalDevice device) {
    // Contains things like:
    //  . Name
    //  . Type
    //  . Supported Vulkan Version
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    // Queries optional features like:
    //  . 64-bit floats
    //  . Multi-viewport rendering
    //  . Texture compression
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(device, &device_features);

    // This basically means:
    //  . The GPU is a dedicated GPU (not an integrated GPU on the CPU [like the one my laptop has])
    //  . The GPU supports Geometry Shaders
    // In this situation both are required
    //return device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && device_features.geometryShader;

    // For now, just accept the first device (I only have one in this laptop anyways)
    // As stated by the tutorial: "Because we're just starting out, Vulkan support is the only thing we need and therefore we'll settle for just any GPU"
    //return true;

    // Its now time for QueueFamilies!
    const QueueFamilyIndices indices = findQueueFamilies(device);
    return indices.isComplete();
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    create_info = {};
    create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = debugCallback;
}

VkResult createDebugUtilsMessengerEXT(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks*              allocator,
    VkDebugUtilsMessengerEXT*                 debug_messenger) {
    auto* func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func != nullptr)
        return func(instance, create_info, allocator, debug_messenger);

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void destroyDebugUtilsMessengerEXT(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     debug_messenger,
    const VkAllocationCallbacks* allocator) {
    auto* func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (func != nullptr)
        func(instance, debug_messenger, allocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    // VERY GOOD REFERENCE FOR THIS: https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Validation_layers

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        spdlog::error(" . Validation Layer Error: {}", pCallbackData->pMessage);

    return VK_FALSE;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;

    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_properties{queue_count};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_count, queue_properties.data());

    auto i = 0;
    for (const auto& [queueFlags, queueCount, timestampValidBits, minImageTransferGranularity] : queue_properties) {
        if (queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.GraphicsFamilyQueue = i;

        VkBool32 presentation_supported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, vk_surface, &presentation_supported);

        if (presentation_supported)
            indices.PresentationFamilyQueue = i;

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

void createLogicalDevice() {
    const QueueFamilyIndices indices = findQueueFamilies(vk_physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos{};
    std::set<uint32_t> unique_queue_families = {indices.GraphicsFamilyQueue.value(), indices.PresentationFamilyQueue.value()};

    float_t queue_priority = 1.0f;
    for (auto queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};

        queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount       = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures features{};

    VkDeviceCreateInfo device_create_info{};

    device_create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos     = queue_create_infos.data();
    device_create_info.queueCreateInfoCount  = static_cast<uint32_t>(queue_create_infos.size());
    device_create_info.pEnabledFeatures      = &features;
    device_create_info.enabledExtensionCount = 0;

    // Used in older implementations, they are global now (and most will ignore them)
    // Just here for legacy compatability reasons
    if (enable_validation_layers) {
        device_create_info.enabledLayerCount   = static_cast<uint32_t>(validation_layers.size());
        device_create_info.ppEnabledLayerNames = validation_layers.data();
    } else
        device_create_info.enabledLayerCount = 0;

    if (vkCreateDevice(vk_physical_device, &device_create_info, nullptr, &vk_logical_device) != VK_SUCCESS)
        throw std::runtime_error{"Failed to create Logical Device!"};

    vkGetDeviceQueue(vk_logical_device, indices.GraphicsFamilyQueue.value(),     0, &vk_graphics_queue);
    vkGetDeviceQueue(vk_logical_device, indices.PresentationFamilyQueue.value(), 0, &vk_presentation_queue);
}

void createSurface() {
    if (glfwCreateWindowSurface(vk_instance, window, nullptr, &vk_surface) != VK_SUCCESS)
        throw std::runtime_error{"Failed to create window surface!"};
}
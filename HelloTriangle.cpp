#include "HelloTriangle.hpp"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#include "spdlog/spdlog.h"

// Two and a half hours, 264-284 lines in, and im only halfway done with setup...
// I still have Presentation, Graphics Pipeline Basics, and Drawing AS MAIN TABS left
//  . I literally haven't even selected a GPU to do anything with yet!

// For reference on how much different (and harder) this is to OpenGL, here is a link to the HelloTriangle for OpenGL
// . https://learnopengl.com/code_viewer_gh.php?code=src/1.getting_started/2.2.hello_triangle_indexed/hello_triangle_indexed.cpp

namespace {
    GLFWwindow* window = nullptr;

    uint32_t width  = 1080;
    uint32_t height = 720;

    // Vulkan
    VkInstance               vk_instance;
    VkDebugUtilsMessengerEXT vk_debug_messenger;

    std::vector<const char*> validation_layers = {
        "VK_LAYER_KHRONOS_validation"
    };

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;

#endif
}

void initWindow();
void initVulkan();
void mainLoop();
void cleanup();

uint32_t helloTriangle() {
    // Vulkan make-sure-it-works code
    /*glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan window", nullptr, nullptr);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::cout << extensionCount << " extensions supported\n";

    glm::mat4 matrix;
    glm::vec4 vec;
    auto test = matrix * vec;

    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();*/

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
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);

// Vulkan Creation
void createInstance();
void setupDebugMessenger();

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
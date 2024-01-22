#include "HelloTriangle.hpp"

#include <cmath>
#include <set>
#include <fstream>
#include <cstring>

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "spdlog/spdlog.h"

#include "StandardUtils.hpp"
#include "VulkanUtilities/BufferUtils.hpp"
#include "VulkanUtilities/DebugUtils.hpp"
#include "VulkanUtilities/ExtensionUtils.hpp"
#include "VulkanUtilities/ShaderUtils.hpp"

namespace HelloTriangle {
    bool  QueueFamilyIndices::isComplete() const { return GraphicsFamilyQueue.has_value() && PresentationFamilyQueue.has_value(); }

    std::array<VkVertexInputAttributeDescription, 2> Vertex::getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> descriptions{};

        // Basically:
        //  . Binding:  Which bound vertex array will the data for this attribute come from
        //  . Location: Which location (in the shader) is this data bound to
        //  . Format:   What is the data type (expressed in colors, unforunately) of this component
        //  . Offset:   What is the offset into the vertex that this data can be found

        // For a list of the Formats, check here for a refresher:
        //  . https://vulkan-tutorial.com/Vertex_buffers/Vertex_input_description

        descriptions[0].binding  = 0;
        descriptions[0].location = 0;
        descriptions[0].format   = VK_FORMAT_R32G32_SFLOAT;
        descriptions[0].offset   = offsetof(Vertex, Position);

        descriptions[1].binding  = 0;
        descriptions[1].location = 1;
        descriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        descriptions[1].offset   = offsetof(Vertex, Color);

        return descriptions;
    }

    VkVertexInputBindingDescription Vertex::getBindingDescription() {
        VkVertexInputBindingDescription description{};

        description.binding   = 0;
        description.stride    = sizeof(Vertex);
        description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // VK_VERTEX_INPUT_RATE_INSTANCE for instanced rendering

        return description;
    }

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

    // Method Implementations
    void initWindow() {
        // Lets just assume everything works with GLFW for now...

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(width, height, "Hello Triangle - Vulkan", nullptr, nullptr);
        glfwSetFramebufferSizeCallback(window, framebufferResized);
    }
    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        selectPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createIndexBuffer();
        createCommandBuffers();
        createSyncObjects();
    }
    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(vk_logical_device);
    }
    void cleanup() {
        if (enable_validation_layers) {
            VulkanUtilities::destroyDebugUtilsMessengerEXT(vk_instance, vk_debug_messenger, nullptr);
        }

        cleanupSwapChain();

        //vkDestroySemaphore(vk_logical_device, image_available_semaphore, nullptr);
        //vkDestroySemaphore(vk_logical_device, render_finished_semaphore, nullptr);
        //vkDestroyFence(vk_logical_device, in_flight_fence, nullptr);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore (vk_logical_device, image_available_semaphores[i], nullptr);
            vkDestroySemaphore (vk_logical_device, render_finished_semaphores[i], nullptr);
            vkDestroyFence     (vk_logical_device, in_flight_fences[i], nullptr);
        }

        vkDestroyCommandPool(vk_logical_device, vk_command_pool, nullptr);

        vkDestroyBuffer(vk_logical_device, vk_vertex_buffer, nullptr);
        vkFreeMemory(vk_logical_device, vk_vertex_memory, nullptr);

        vkDestroyBuffer(vk_logical_device, vk_index_buffer, nullptr);
        vkFreeMemory(vk_logical_device, vk_index_memory, nullptr);

        vkDestroyRenderPass(vk_logical_device, vk_render_pass, nullptr);
        vkDestroyPipeline(vk_logical_device, vk_pipeline, nullptr);
        vkDestroyPipelineLayout(vk_logical_device, vk_pipeline_layout, nullptr);
        vkDestroyDevice(vk_logical_device, nullptr);
        vkDestroySurfaceKHR(vk_instance, vk_surface, nullptr);
        vkDestroyInstance(vk_instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void framebufferResized(GLFWwindow* window, int width, int height) {
        framebuffer_resized = true;
    }

    void drawFrame() {
        static uint32_t current_frame = 0;

        vkWaitForFences(vk_logical_device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

        uint32_t image_index = 0;
        VkResult result = vkAcquireNextImageKHR(vk_logical_device, vk_swapchain, UINT64_MAX, image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            throw std::runtime_error{"Failed to aquire swapchain image!"};

        // This has been moved down here to prevent dealocks for when VK_ERROR_OUT_OF_DATE_KHR occurs
        vkResetFences(vk_logical_device, 1, &in_flight_fences[current_frame]);

        vkResetCommandBuffer(vk_command_buffers[current_frame], 0);
        recordCommandBuffer(vk_command_buffers[current_frame], image_index);

        VkSubmitInfo submit_info{};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore          wait_semaphores[] = {image_available_semaphores[current_frame]};
        VkPipelineStageFlags wait_stages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submit_info.waitSemaphoreCount         = 1;
        submit_info.pWaitSemaphores            = wait_semaphores;
        submit_info.pWaitDstStageMask          = wait_stages;
        submit_info.commandBufferCount         = 1;
        submit_info.pCommandBuffers            = &vk_command_buffers[current_frame];

        VkSemaphore signal_semaphores[]  = {render_finished_semaphores[current_frame]};
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores    = signal_semaphores;

        if (vkQueueSubmit(vk_graphics_queue, 1, &submit_info, in_flight_fences[current_frame]) != VK_SUCCESS)
            throw std::runtime_error{"Failed to submit the draw command buffer!"};

        VkPresentInfoKHR present_info{};

        present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = signal_semaphores;

        const VkSwapchainKHR swap_chains[] = {vk_swapchain};
        present_info.swapchainCount        = 1;
        present_info.pSwapchains           = swap_chains;
        present_info.pImageIndices         = &image_index;
        present_info.pResults              = nullptr; // Optional

        result = vkQueuePresentKHR(vk_graphics_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS)
            throw std::runtime_error{"Failed to present the swapchain image"};

        current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
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
            layer_count = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());;
            layers = VK_VALIDATION_LAYERS.data();
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

        for (auto* layer_name : VK_VALIDATION_LAYERS) {
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

        if (VulkanUtilities::createDebugUtilsMessengerEXT(vk_instance, &create_info, nullptr, &vk_debug_messenger) != VK_SUCCESS)
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

        const bool extensions_supported = checkDeviceExtensionSupport(device);

        bool swapchain_adaquate = false;
        if (extensions_supported) {
            auto [Capabilities, SurfaceFormats, PresentModes] = querySwapChainSupport(device);
            swapchain_adaquate = !SurfaceFormats.empty() && !PresentModes.empty();
        }

        // Its now time for QueueFamilies!
        const QueueFamilyIndices indices = findQueueFamilies(device);
        return indices.isComplete() && extensions_supported && swapchain_adaquate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extension_count = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions{extension_count};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions{VK_REQUIRED_EXTENSIONS.begin(), VK_REQUIRED_EXTENSIONS.end()};

        for (const auto& [extensionName, specVersion] : available_extensions)
            required_extensions.erase(extensionName);

        return required_extensions.empty();
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info) {
        create_info = {};
        create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = VulkanUtilities::debugCallback;
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

        device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pQueueCreateInfos       = queue_create_infos.data();
        device_create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
        device_create_info.pEnabledFeatures        = &features;
        device_create_info.enabledExtensionCount   = static_cast<uint32_t>(VK_REQUIRED_EXTENSIONS.size());
        device_create_info.ppEnabledExtensionNames = VK_REQUIRED_EXTENSIONS.data();

        // Used in older implementations, they are global now (and most will ignore them)
        // Just here for legacy compatability reasons
        if (enable_validation_layers) {
            device_create_info.enabledLayerCount   = static_cast<uint32_t>(VK_VALIDATION_LAYERS.size());
            device_create_info.ppEnabledLayerNames = VK_VALIDATION_LAYERS.data();
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

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vk_surface, &details.Capabilities);

        uint32_t format_count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface, &format_count, nullptr);

        if (format_count != 0) {
            details.SurfaceFormats.resize(format_count);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk_surface, &format_count, details.SurfaceFormats.data());
        }

        uint32_t present_count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface, &present_count, nullptr);

        if (present_count != 0) {
            details.PresentModes.resize(present_count);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk_surface, &present_count, details.PresentModes.data());
        }


        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFomat(const std::vector<VkSurfaceFormatKHR>& available_formats) {
        for (const auto& available_format : available_formats) {
            if (available_format.format     == VK_FORMAT_B8G8R8A8_SRGB &&
                available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return available_format;
        }

        return available_formats[0];
    }

    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) {
        // Here is a good reference for what the different types are and what they do:
        //  . https://vulkan-tutorial.com/en/Drawing_a_triangle/Presentation/Swap_chain

        // If we have Triple-Buffering support, choose it at the current moment (as per the tutorial)
        // It offers (at the expense of energy) the lowest latency rendering mode without tearing
        //  . VK_PRESENT_MODE_MAILBOX_KHR
        for (const auto& available_mode : available_present_modes)
            if (available_mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return VK_PRESENT_MODE_MAILBOX_KHR;

        // The standard Double-Buffering seen in OpenGL
        //  . Terrific for mobile devices (or devices where power is a concern)
        //  . Guaranteed to be present. The others... No so much.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actual_extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

        actual_extent.width  = std::clamp(actual_extent.width,  capabilities.minImageExtent.width,  capabilities.maxImageExtent.width);
        actual_extent.height = std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actual_extent;
    }

    void createSwapChain() {
        SwapChainSupportDetails support_details = querySwapChainSupport(vk_physical_device);

        VkSurfaceFormatKHR surface_format = chooseSwapSurfaceFomat(support_details.SurfaceFormats);
        VkPresentModeKHR   present_mode   = choosePresentMode(support_details.PresentModes);
        VkExtent2D         extent         = chooseSwapExtent(support_details.Capabilities);

        uint32_t image_count = support_details.Capabilities.minImageCount + 1;
        if (support_details.Capabilities.maxImageCount > 0 && image_count > support_details.Capabilities.maxImageCount)
            image_count = support_details.Capabilities.maxImageCount;

        VkSwapchainCreateInfoKHR create_info{};

        create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface          = vk_surface;
        create_info.minImageCount    = image_count;
        create_info.imageFormat      = surface_format.format;
        create_info.imageColorSpace  = surface_format.colorSpace;
        create_info.imageExtent      = extent;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const QueueFamilyIndices indices                = findQueueFamilies(vk_physical_device);
        const uint32_t           queue_family_indices[] = {indices.GraphicsFamilyQueue.value(), indices.PresentationFamilyQueue.value() };

        if (indices.GraphicsFamilyQueue != indices.PresentationFamilyQueue) {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices   = queue_family_indices;
        } else {
            create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            create_info.queueFamilyIndexCount = 0;
            create_info.pQueueFamilyIndices   = nullptr;
        }

        create_info.preTransform   = support_details.Capabilities.currentTransform;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode    = present_mode;
        create_info.clipped        = VK_TRUE;
        create_info.oldSwapchain   = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(vk_logical_device, &create_info, nullptr, &vk_swapchain) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create the Swapchain!"};

        vkGetSwapchainImagesKHR(vk_logical_device, vk_swapchain, &image_count, nullptr);
        vk_swapchain_images.resize(image_count);
        vkGetSwapchainImagesKHR(vk_logical_device, vk_swapchain, &image_count, vk_swapchain_images.data());

        vk_swapchain_image_format = surface_format.format;
        vk_swapchain_extent       = extent;
    }

    void createImageViews() {
        vk_swapchain_image_views.resize(vk_swapchain_images.size());

        for (size_t i = 0; i < vk_swapchain_images.size(); i++) {
            VkImageViewCreateInfo create_info{};

            create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image    = vk_swapchain_images[i];
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format   = vk_swapchain_image_format;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;

            if (vkCreateImageView(vk_logical_device, &create_info, nullptr, &vk_swapchain_image_views[i]) != VK_SUCCESS)
                throw std::runtime_error{"Failed to create Image Views!"};
        }
    }

    void createGraphicsPipeline() {
        const auto vertex_bytecode   = StandardUtilities::readFile("res/vert.spv");
        const auto fragment_bytecode = StandardUtilities::readFile("res/frag.spv");

        VkShaderModule vertex_shader_module   = VulkanUtilities::createShaderModule(vk_logical_device, vertex_bytecode);
        VkShaderModule fragment_shader_module = VulkanUtilities::createShaderModule(vk_logical_device, fragment_bytecode);

        VkPipelineShaderStageCreateInfo vertex_create_info{};

        vertex_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertex_create_info.stage               = VK_SHADER_STAGE_VERTEX_BIT;
        vertex_create_info.module              = vertex_shader_module;
        vertex_create_info.pName               = "main";
        vertex_create_info.pSpecializationInfo = nullptr; // Default value, but really good for constant values!

        VkPipelineShaderStageCreateInfo fragment_create_info{};

        fragment_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_create_info.stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_create_info.module              = fragment_shader_module;
        fragment_create_info.pName               = "main";
        fragment_create_info.pSpecializationInfo = nullptr; // Default value, but really good for constant values!

        VkPipelineShaderStageCreateInfo shader_stages[] = { vertex_create_info, fragment_create_info };

        auto vertex_binding_description    = Vertex::getBindingDescription();
        auto vertex_attribute_descriptions = Vertex::getAttributeDescriptions();

        VkPipelineVertexInputStateCreateInfo vertex_input_info{};

        vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertex_input_info.vertexBindingDescriptionCount   = 1;
        vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attribute_descriptions.size());
        vertex_input_info.pVertexBindingDescriptions      = &vertex_binding_description;
        vertex_input_info.pVertexAttributeDescriptions    = vertex_attribute_descriptions.data();

        VkPipelineInputAssemblyStateCreateInfo input_assembly{};

        input_assembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        input_assembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        input_assembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};

        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(vk_swapchain_extent.width);
        viewport.height   = static_cast<float>(vk_swapchain_extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent   = vk_swapchain_extent;

        // There some other stuff about Dynamic states for Scissor and Viewport, but im sticking static for now
        //  . https://vulkan-tutorial.com/en/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
        VkPipelineViewportStateCreateInfo viewport_state{};

        viewport_state.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport_state.viewportCount = 1;
        viewport_state.pViewports    = &viewport;
        viewport_state.scissorCount  = 1;
        viewport_state.pScissors     = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};

        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable        = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth               = 1.0f; // Required for line modes (dont forget this, it broke it earlier)
        rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable         = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp          = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor    = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};

        multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable   = VK_FALSE;
        multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading      = 1.0f;     // Optional
        multisampling.pSampleMask           = nullptr;  // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable      = VK_FALSE; // Optional

        // Not doing Depth or Stencil testing (yet), so ignore it for now

        VkPipelineColorBlendAttachmentState color_blend_attachment{};

        color_blend_attachment.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachment.blendEnable         = VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        color_blend_attachment.colorBlendOp        = VK_BLEND_OP_ADD;      // Optional
        color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        color_blend_attachment.alphaBlendOp        = VK_BLEND_OP_ADD;      // Optional

        VkPipelineColorBlendStateCreateInfo color_blending{};
        color_blending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        color_blending.logicOpEnable     = VK_FALSE;
        color_blending.logicOp           = VK_LOGIC_OP_COPY; // Optional
        color_blending.attachmentCount   = 1;
        color_blending.pAttachments      = &color_blend_attachment;
        color_blending.blendConstants[0] = 0.0f; // Optional
        color_blending.blendConstants[1] = 0.0f; // Optional
        color_blending.blendConstants[2] = 0.0f; // Optional
        color_blending.blendConstants[3] = 0.0f; // Optional

        std::vector<VkDynamicState> dynamic_states = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamic_state{};
        dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic_state.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineLayoutCreateInfo pipeline_layout_info{};

        pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_info.setLayoutCount         = 0;       // Optional
        pipeline_layout_info.pSetLayouts            = nullptr; // Optional
        pipeline_layout_info.pushConstantRangeCount = 0;       // Optional
        pipeline_layout_info.pPushConstantRanges    = nullptr; // Optional

        if (vkCreatePipelineLayout(vk_logical_device, &pipeline_layout_info, nullptr, &vk_pipeline_layout) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create Pipeline Layout!"};

        VkGraphicsPipelineCreateInfo graphics_pipeline_info{};

        graphics_pipeline_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        graphics_pipeline_info.stageCount          = 2;
        graphics_pipeline_info.pStages             = shader_stages;
        graphics_pipeline_info.pVertexInputState   = &vertex_input_info;
        graphics_pipeline_info.pInputAssemblyState = &input_assembly;
        graphics_pipeline_info.pViewportState      = &viewport_state;
        graphics_pipeline_info.pRasterizationState = &rasterizer;
        graphics_pipeline_info.pMultisampleState   = &multisampling;
        graphics_pipeline_info.pDepthStencilState  = nullptr; // Optional
        graphics_pipeline_info.pColorBlendState    = &color_blending;
        graphics_pipeline_info.pDynamicState       = &dynamic_state;
        graphics_pipeline_info.layout              = vk_pipeline_layout;
        graphics_pipeline_info.renderPass          = vk_render_pass;
        graphics_pipeline_info.subpass             = 0;
        graphics_pipeline_info.basePipelineHandle  = VK_NULL_HANDLE; // Optional
        graphics_pipeline_info.basePipelineIndex   = -1;             // Optional

        if (vkCreateGraphicsPipelines(vk_logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &vk_pipeline) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create the Graphics Pipeline!"};


        // Ending
        vkDestroyShaderModule(vk_logical_device, vertex_shader_module,   nullptr);
        vkDestroyShaderModule(vk_logical_device, fragment_shader_module, nullptr);
    }

    void createRenderPass() {
        VkAttachmentDescription color_attachment{};

        color_attachment.format         = vk_swapchain_image_format;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_reference{};

        color_attachment_reference.attachment = 0;
        color_attachment_reference.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass_description{};

        subpass_description.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.colorAttachmentCount = 1;
        subpass_description.pColorAttachments    = &color_attachment_reference;

        VkSubpassDependency dependency{};

        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info{};

        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments    = &color_attachment;
        render_pass_info.subpassCount    = 1;
        render_pass_info.pSubpasses      = &subpass_description;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies   = &dependency;

        if (vkCreateRenderPass(vk_logical_device, &render_pass_info, nullptr, &vk_render_pass) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create the Render Pass!"};
    }

    void createFramebuffers() {
        vk_swapchain_framebuffers.resize(vk_swapchain_image_views.size());


        for (size_t i = 0; i < vk_swapchain_image_views.size(); i++) {
            VkImageView attachments[] = { vk_swapchain_image_views[i] };

            VkFramebufferCreateInfo framebuffer_info{};

            framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;;
            framebuffer_info.renderPass      = vk_render_pass;
            framebuffer_info.attachmentCount = 1;
            framebuffer_info.pAttachments    = attachments;
            framebuffer_info.width           = vk_swapchain_extent.width;
            framebuffer_info.height          = vk_swapchain_extent.height;
            framebuffer_info.layers          = 1;

            if (vkCreateFramebuffer(vk_logical_device, &framebuffer_info, nullptr, &vk_swapchain_framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error{"Failed to create framebuffer!"};
        }
    }

    void createCommandPool() {
        const QueueFamilyIndices indices = findQueueFamilies(vk_physical_device);

        VkCommandPoolCreateInfo create_info{};

        create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        create_info.queueFamilyIndex = indices.GraphicsFamilyQueue.value();

        if (vkCreateCommandPool(vk_logical_device, &create_info, nullptr, &vk_command_pool) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create Command Pool!"};
    }

    void createCommandBuffers() {
        vk_command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocate_info{};

        allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool        = vk_command_pool;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = vk_command_buffers.size();

        if (vkAllocateCommandBuffers(vk_logical_device, &allocate_info, vk_command_buffers.data()) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create Command Buffer!"};
    }

    void createSyncObjects() {
        image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
        in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphore_info{};
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fence_info{};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(vk_logical_device, &semaphore_info, nullptr, &image_available_semaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(vk_logical_device, &semaphore_info, nullptr, &render_finished_semaphores[i]) != VK_SUCCESS ||
                vkCreateFence(vk_logical_device, &fence_info, nullptr, &in_flight_fences[i]) != VK_SUCCESS
                )
                throw std::runtime_error("failed to create frame syncronization objects!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer buffer, uint32_t image_index) {
        VkCommandBufferBeginInfo begin_info{};

        begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags            = 0;       // Optional
        begin_info.pInheritanceInfo = nullptr; // Optional

        if (vkBeginCommandBuffer(buffer, &begin_info) != VK_SUCCESS)
            throw std::runtime_error{"Failed to begin command Buffer"};

        VkRenderPassBeginInfo render_begin_info{};

        render_begin_info.sType               = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_begin_info.renderPass          = vk_render_pass;
        render_begin_info.framebuffer         = vk_swapchain_framebuffers[image_index];
        render_begin_info.renderArea.offset = {0, 0};
        render_begin_info.renderArea.extent   = vk_swapchain_extent;

        const VkClearValue clear_value{{{0.0f, 0.0f, 0.0f, 1.0f}}};

        render_begin_info.clearValueCount = 1;
        render_begin_info.pClearValues    = &clear_value;

        vkCmdBeginRenderPass(buffer, &render_begin_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_pipeline);

        VkViewport viewport{};

        viewport.x        = 0.0f;
        viewport.y        = 0.0f;
        viewport.width    = static_cast<float>(vk_swapchain_extent.width);
        viewport.height   = static_cast<float>(vk_swapchain_extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vkCmdSetViewport(buffer, 0, 1, &viewport);

        VkRect2D scissor{};

        scissor.offset = {0, 0};
        scissor.extent   = vk_swapchain_extent;

        vkCmdSetScissor(buffer, 0, 1, &scissor);

        const VkBuffer     vertex_buffers[] { vk_vertex_buffer };
        const VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(buffer, 0, 1, vertex_buffers, offsets);
        vkCmdBindIndexBuffer(buffer, vk_index_buffer, 0, VK_INDEX_TYPE_UINT16);

        // THIS IS IT! ITS TIME FOR THE TRIANGLE!!!!!!! [now a rectangle]
        vkCmdDrawIndexed(buffer, static_cast<uint32_t>(INDICES.size()), 1, 0, 0, 0);

        vkCmdEndRenderPass(buffer);

        if (vkEndCommandBuffer(buffer) != VK_SUCCESS)
            throw std::runtime_error{"Failed to record command buffer!"};
    }

    void recreateSwapChain() {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(vk_logical_device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    void cleanupSwapChain() {
        for (const auto framebuffer : vk_swapchain_framebuffers)
            vkDestroyFramebuffer(vk_logical_device, framebuffer, nullptr);

        for (const auto view : vk_swapchain_image_views)
            vkDestroyImageView(vk_logical_device, view, nullptr);

        vkDestroySwapchainKHR(vk_logical_device, vk_swapchain, nullptr);
    }

    void createVertexBuffer() {
        const VkDeviceSize memory_size = sizeof(VERTICES[0]) * VERTICES.size();

        VkBuffer       staging_buffer;
        VkDeviceMemory staging_memory;
        VulkanUtilities::createBuffer(vk_logical_device, vk_physical_device, memory_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, staging_buffer, staging_memory);

        void* data;
        vkMapMemory(vk_logical_device, staging_memory, 0, memory_size, 0, &data);
        memcpy(data, VERTICES.data(), memory_size);
        vkUnmapMemory(vk_logical_device, staging_memory);

        VulkanUtilities::createBuffer(vk_logical_device, vk_physical_device, memory_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_vertex_buffer, vk_vertex_memory);

        VulkanUtilities::copyBuffer(vk_logical_device, vk_command_pool, vk_graphics_queue, staging_buffer, vk_vertex_buffer, memory_size);

        vkDestroyBuffer(vk_logical_device, staging_buffer, nullptr);
        vkFreeMemory(vk_logical_device, staging_memory, nullptr);

        /*const VkDeviceSize memory_size = sizeof(vertices[0]) * vertices.size();
        createBuffer(memory_size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vk_vertex_buffer, vk_vertex_memory);

        void* data;
        vkMapMemory(vk_logical_device, vk_vertex_memory, 0, memory_size, 0, &data);
        memcpy(data, vertices.data(), memory_size);
        vkUnmapMemory(vk_logical_device, vk_vertex_memory);*/
    }


    void createIndexBuffer() {
        const VkDeviceSize memory_size = sizeof(INDICES[0]) * INDICES.size();

        VkBuffer       staging_buffer;
        VkDeviceMemory staging_memory;
        VulkanUtilities::createBuffer(vk_logical_device, vk_physical_device, memory_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, staging_buffer, staging_memory);

        void* data;
        vkMapMemory(vk_logical_device, staging_memory, 0, memory_size, 0, &data);
        memcpy(data, INDICES.data(), memory_size);
        vkUnmapMemory(vk_logical_device, staging_memory);

        VulkanUtilities::createBuffer(vk_logical_device, vk_physical_device, memory_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vk_index_buffer, vk_index_memory);

        VulkanUtilities::copyBuffer(vk_logical_device, vk_command_pool, vk_graphics_queue, staging_buffer, vk_index_buffer, memory_size);

        vkDestroyBuffer(vk_logical_device, staging_buffer, nullptr);
        vkFreeMemory(vk_logical_device, staging_memory, nullptr);
    }
}
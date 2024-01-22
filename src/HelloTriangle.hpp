#pragma once
#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "StandardUtils.hpp"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp"

// Initial Learning of Vulkan (Chapter 1)

namespace HelloTriangle {
    // Structures
    struct Vertex {
        glm::vec2 Position;
        glm::vec3 Color;

        static VkVertexInputBindingDescription                  getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions();
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> GraphicsFamilyQueue;
        std::optional<uint32_t> PresentationFamilyQueue;

        [[nodiscard]] bool isComplete() const;
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR        Capabilities;
        std::vector<VkSurfaceFormatKHR> SurfaceFormats;
        std::vector<VkPresentModeKHR>   PresentModes;
    };

    // General Variables
    inline GLFWwindow* window = nullptr;

    inline uint32_t width  = 1080;
    inline uint32_t height = 720;

    inline bool framebuffer_resized = false;

    // Vulkan Constants
    inline constexpr uint32_t                 MAX_FRAMES_IN_FLIGHT = 2;
    inline std::vector<const char*> VK_VALIDATION_LAYERS = {
        "VK_LAYER_KHRONOS_validation"
    };
    inline const std::vector<const char*> VK_REQUIRED_EXTENSIONS = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // I really hate that Y is inverted so that y- is the top of the screen...
    inline const std::vector<Vertex> VERTICES = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
    };

    inline const std::vector<uint16_t> INDICES = { 0, 1, 2, 2, 3, 0 };

#ifdef NDEBUG
    const bool enable_validation_layers = false;
#else
    const bool enable_validation_layers = true;
#endif

    // Vulkan Variables
    inline VkInstance               vk_instance;
    inline VkDebugUtilsMessengerEXT vk_debug_messenger;
    inline VkSurfaceKHR             vk_surface;
    inline VkPhysicalDevice         vk_physical_device = VK_NULL_HANDLE; // Implicitly Destroyed
    inline VkDevice                 vk_logical_device;
    inline VkQueue                  vk_graphics_queue;
    inline VkQueue                  vk_presentation_queue;
    inline VkSwapchainKHR           vk_swapchain;
    inline VkRenderPass             vk_render_pass;
    inline VkPipelineLayout         vk_pipeline_layout;
    inline VkPipeline               vk_pipeline;
    inline VkCommandPool            vk_command_pool;

    inline std::vector<VkImage>       vk_swapchain_images;
    inline std::vector<VkImageView>   vk_swapchain_image_views;
    inline VkFormat                   vk_swapchain_image_format;
    inline VkExtent2D                 vk_swapchain_extent;
    inline std::vector<VkFramebuffer> vk_swapchain_framebuffers;

    inline std::vector<VkCommandBuffer> vk_command_buffers;

    inline std::vector<VkSemaphore> image_available_semaphores;
    inline std::vector<VkSemaphore> render_finished_semaphores;
    inline std::vector<VkFence>     in_flight_fences;

    inline VkBuffer       vk_vertex_buffer;
    inline VkDeviceMemory vk_vertex_memory;
    inline VkBuffer       vk_index_buffer;
    inline VkDeviceMemory vk_index_memory;

    // Entrypoint
    uint32_t helloTriangle();

    // Lifecycle Methods
    void initWindow();
    void initVulkan();
    void mainLoop();
    void drawFrame();
    void cleanup();

    // Event Callbacks
    void framebufferResized(GLFWwindow* window, int width, int height);

    // Vulkan Methods
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();

    void recreateSwapChain();
    void cleanupSwapChain();

    void createVertexBuffer();
    void createIndexBuffer();

    bool                     checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();

    void recordCommandBuffer(VkCommandBuffer buffer, uint32_t image_index);

    VkSurfaceFormatKHR chooseSwapSurfaceFomat(const std::vector<VkSurfaceFormatKHR>& available_formats);
    VkPresentModeKHR   choosePresentMode(const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D         chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

    void                    populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& create_info);
    bool                    isDeviceSuitable(VkPhysicalDevice);
    bool                    checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices      findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
}
#pragma once
#include <vulkan_core.h>

namespace VulkanUtilities {
    // Creation
    VkResult createDebugUtilsMessengerEXT(
        VkInstance                                instance,
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks*              allocator,
        VkDebugUtilsMessengerEXT*                 debug_messenger
    );

    // Destruction
    void destroyDebugUtilsMessengerEXT(
        VkInstance                   instance,
        VkDebugUtilsMessengerEXT     debug_messenger,
        const VkAllocationCallbacks* allocator
    );
}

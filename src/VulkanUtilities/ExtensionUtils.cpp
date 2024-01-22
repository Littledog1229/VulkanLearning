#include "VulkanUtilities/ExtensionUtils.hpp"

namespace VulkanUtilities {
    // Creation
    VkResult createDebugUtilsMessengerEXT(
        VkInstance                                instance,
        const VkDebugUtilsMessengerCreateInfoEXT* create_info,
        const VkAllocationCallbacks*              allocator,
        VkDebugUtilsMessengerEXT*                 debug_messenger
    ) {
        auto* func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

        if (func != nullptr)
            return func(instance, create_info, allocator, debug_messenger);

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    // Destruction
    void destroyDebugUtilsMessengerEXT(
        VkInstance                   instance,
        VkDebugUtilsMessengerEXT     debug_messenger,
        const VkAllocationCallbacks* allocator
    ) {
        auto* func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr)
            func(instance, debug_messenger, allocator);
    }
}
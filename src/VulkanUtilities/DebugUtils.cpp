#include "VulkanUtilities/DebugUtils.hpp"

#include "spdlog/spdlog.h"

namespace VulkanUtilities {
    VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        // VERY GOOD REFERENCE FOR THIS: https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Validation_layers

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
            spdlog::error(" . Validation Layer Error: {}", pCallbackData->pMessage);

        return VK_FALSE;
    }
}

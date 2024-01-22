#pragma once

#include <vulkan_core.h>

namespace VulkanUtilities {
    uint32_t findMemoryType(
        VkPhysicalDevice      device,
        uint32_t              type_filter,
        VkMemoryPropertyFlags properties
    );

    void createBuffer(
        VkDevice              device,
        VkPhysicalDevice      physical_device,
        VkDeviceSize          size,
        VkBufferUsageFlags    usage_flags,
        VkMemoryPropertyFlags property_flags,
        VkBuffer&             buffer,
        VkDeviceMemory&       memory
    );

    void copyBuffer(
        VkDevice      device,
        VkCommandPool pool,
        VkQueue       queue,
        VkBuffer      source,
        VkBuffer      destination,
        VkDeviceSize  size
    );
}

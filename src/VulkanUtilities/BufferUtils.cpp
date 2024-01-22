#include "VulkanUtilities/BufferUtils.hpp"

#include <stdexcept>

namespace VulkanUtilities {
    uint32_t findMemoryType(
        const VkPhysicalDevice      device,
        const uint32_t              type_filter,
        const VkMemoryPropertyFlags properties
    ) {
        VkPhysicalDeviceMemoryProperties physical_memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(device, &physical_memory_properties);

        for (uint32_t i = 0; i < physical_memory_properties.memoryTypeCount; i++) {
            if (type_filter & (1 << i) &&
                (physical_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
                return i;
        }

        throw std::runtime_error{"Failed to find suitable memory type!"};
    }

    void createBuffer(
        const VkDevice              device,
        const VkPhysicalDevice      physical_device,
        const VkDeviceSize          size,
        const VkBufferUsageFlags    usage_flags,
        const VkMemoryPropertyFlags property_flags,
        VkBuffer&                   buffer,
        VkDeviceMemory&             memory
    ) {
        VkBufferCreateInfo create_info{};

        create_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        create_info.size        = size;
        create_info.usage       = usage_flags;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &create_info, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create buffer!"};

        VkMemoryRequirements memory_requirements{};
        vkGetBufferMemoryRequirements(device, buffer, &memory_requirements);

        VkMemoryAllocateInfo allocate_info{};

        allocate_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocate_info.allocationSize  = memory_requirements.size;
        allocate_info.memoryTypeIndex = findMemoryType(physical_device, memory_requirements.memoryTypeBits, property_flags);

        if (vkAllocateMemory(device, &allocate_info, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error{"Failed to allocate buffer memory!"};

        vkBindBufferMemory(device, buffer, memory, 0);
    }

    void copyBuffer(
        const VkDevice      device,
        const VkCommandPool pool,
        const VkQueue       queue,
        const VkBuffer      source,
        const VkBuffer      destination,
        const VkDeviceSize  size
    ) {
        VkCommandBufferAllocateInfo allocate_info{};

        allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandPool        = pool;
        allocate_info.commandBufferCount = 1;

        VkCommandBuffer command_buffer;
        vkAllocateCommandBuffers(device, &allocate_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info{};

        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(command_buffer, &begin_info);

        VkBufferCopy copy_region{};

        copy_region.srcOffset = 0; // Optional
        copy_region.dstOffset = 0; // Optional
        copy_region.size      = size;

        vkCmdCopyBuffer(command_buffer, source, destination, 1, &copy_region);

        vkEndCommandBuffer(command_buffer);

        VkSubmitInfo submit_info{};

        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;

        vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        vkFreeCommandBuffers(device, pool, 1, &command_buffer);
    }
}

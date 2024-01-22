#pragma once

#include <vector>
#include <vulkan_core.h>

namespace VulkanUtilities {
    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& shader);
}

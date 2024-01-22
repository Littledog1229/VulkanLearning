#include "VulkanUtilities/ShaderUtils.hpp"

#include <stdexcept>

namespace VulkanUtilities {
    VkShaderModule createShaderModule(const VkDevice device, const std::vector<char>& shader) {
        VkShaderModuleCreateInfo create_info{};

        create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = shader.size();
        create_info.pCode    = reinterpret_cast<const uint32_t*>(shader.data());

        VkShaderModule shader_module;
        if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) != VK_SUCCESS)
            throw std::runtime_error{"Failed to create Shader Module!"};

        return shader_module;
    }
}

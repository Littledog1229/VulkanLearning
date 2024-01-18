cmake_minimum_required(VERSION 3.27)
project(VulkanLearning)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(extern/glm)
add_subdirectory(extern/glfw)
add_subdirectory(extern/spdlog)

add_executable(VulkanLearning main.cpp
        HelloTriangle.cpp
        HelloTriangle.hpp)

target_include_directories(VulkanLearning PUBLIC extern/glfw/include)
target_include_directories(VulkanLearning PUBLIC extern/glm/glm)
target_include_directories(VulkanLearning PUBLIC extern/spdlog/include)

include_directories(VulkanLearning extern/vulkan)
include_directories(VulkanLearning extern/vulkan/vulkan)

target_link_libraries(VulkanLearning glfw)
target_link_libraries(VulkanLearning glm)
target_link_libraries(VulkanLearning spdlog)
target_link_libraries(VulkanLearning ${CMAKE_CURRENT_SOURCE_DIR}/extern/lib/vulkan-1.lib)
cmake_minimum_required(VERSION 3.10)

project(MyVulkanProject)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include/vulkan)
include_directories(${CMAKE_SOURCE_DIR}/include/glfw)


# Add the lib directory to the link directories
link_directories(${CMAKE_SOURCE_DIR}/lib)


add_executable(VulkanApp src/main.c)

# Link Vulkan and GLFW libraries
target_link_libraries(VulkanApp PRIVATE vulkan-1 glfw3)
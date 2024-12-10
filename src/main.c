#include <vulkan.h>
#include <glfw3.h>
#include <stdio.h>
#include <stdlib.h>


GLFWwindow* create_window() {
    glfwInit();
    // dont use openGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // don't allow resizing, for now
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    return glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
}

void destroy_window(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

VkInstance create_vulkan_instance() {
    // optional, but helps with optimisation
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    // required
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
    create_info.enabledLayerCount = 0;
    VkInstance instance = VK_NULL_HANDLE;
    if(vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
        return VK_NULL_HANDLE;
    }
    return instance;
}

void destroy_vulkan_instance(VkInstance instance) {
    vkDestroyInstance(instance, NULL);
}

int main(){
    GLFWwindow* window = create_window();
    VkInstance vk_instance = create_vulkan_instance();
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
    destroy_vulkan_instance(vk_instance);
    destroy_window(window);
    return EXIT_SUCCESS;
}
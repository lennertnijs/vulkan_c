#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int enable_validation_layers = 1;
const int validation_layer_count = 1;
const char* validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

GLFWwindow* create_window() {
    if(glfwInit() == 0){
		printf("Glfw context initialisation failed.\n");
		return NULL;
	};
    // dont use openGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	if(window == NULL){
		printf("Glfw window creation failed.\n");
		glfwTerminate();
		return NULL;
	}
	return window;
}

void destroy_window(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

int check_validation_layer_support(){
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, NULL);
    VkLayerProperties* layer_properties_list = malloc(sizeof(VkLayerProperties) * layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, layer_properties_list);
    
    for(int i = 0; i < validation_layer_count; i++){
		int  layer_supported = 0;
		for(int j = 0; j < layer_count; j++){
			if(strcmp(validation_layers[i], layer_properties_list[j].layerName) == 0){
				layer_supported = 1;
				break;
			}
		}
		if(layer_supported == 0){
			return 0;
		}
    }
    return 1;
}

VkInstance create_vulkan_instance() {
	if(enable_validation_layers != 0 && check_validation_layer_support() == 0){
		printf("Validation layers are not supported.\n");
		return NULL;
	}
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
	if(enable_validation_layers != 0){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
	}
    VkInstance instance = VK_NULL_HANDLE;
    if(vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
        return NULL;
    }
    return instance;
}

void destroy_vulkan_instance(VkInstance instance) {
    vkDestroyInstance(instance, NULL);
}

int is_suitable_physical_device(VkPhysicalDevice physical_device){
	if(physical_device == NULL){
		return 0;
	}
	return 1;
}

VkPhysicalDevice* pick_physical_device(VkInstance instance){
	// is implicitly destroyed with the vkInstance
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);
	if(physical_device_count == 0){
		return NULL;
	}
	VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(instance, &physical_device_count, physical_devices);
	for(uint32_t i = 0; i < physical_device_count; i++){
		if(is_suitable_physical_device(physical_devices[i]) == 1){
			VkPhysicalDevice* suitable_device = malloc(sizeof(VkPhysicalDevice));
			*suitable_device = physical_devices[i];
			free(physical_devices);
			return suitable_device;
		}
	}
	free(physical_devices);
	return NULL;	
}

int main(){
    GLFWwindow* window = create_window();
	if(window == NULL){
		return 0;
	}
	printf("Window creation succesful!\n");
    VkInstance vk_instance = create_vulkan_instance();
	if(vk_instance == NULL){
		destroy_window(window);
		return 0;
	}
	printf("Vulkan instance creation succesful!\n");
	VkPhysicalDevice* physical_device = pick_physical_device(vk_instance);
	if(physical_device == NULL){
		destroy_vulkan_instance(vk_instance);
		destroy_window(window);
		return 0;
	}
	printf("Physical device selection succesful!\n");
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }
	free(physical_device);
    destroy_vulkan_instance(vk_instance);
    destroy_window(window);
    return 1;
}


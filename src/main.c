#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define SUCCESS 1
#define FAILURE 0

#ifdef DEBUG
	#define log(msg) printf("%s\n", msg)
	const bool enable_validation_layers = true;
#else
	#define log(msg)
	const bool enable_validation_layers = false;
#endif

const int validation_layer_count = 1;
const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const int extension_count = 1;
const char *extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

typedef struct {
	GLFWwindow* window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	uint32_t graphics_queue_index;
	uint32_t present_queue_index;
	VkDevice logical_device;
	VkQueue graphics_queue;
	VkQueue present_queue;
} VkContext;

int create_window(VkContext* context) {
	// dont use openGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	if(window == NULL){
		log("Glfw window creation failed.");
		return FAILURE;
	}
	context->window = window;
	log("Glfw window creation succesful!");
	return SUCCESS;
}

bool supports_validation_layer(){
    uint32_t supported_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);
    VkLayerProperties* supported_layers_list = malloc(sizeof(VkLayerProperties) * supported_layer_count);
    vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers_list);
    
    for(int i = 0; i < validation_layer_count; i++){
		bool  layer_supported = false;
		for(uint32_t j = 0; j < supported_layer_count; j++){
			if(strcmp(validation_layers[i], supported_layers_list[j].layerName) == 0){
				layer_supported = true;
				break;
			}
		}
		if(!layer_supported){
			return false;
		}
    }
    return true;
}

int create_vk_instance(VkContext *context) {
	if(enable_validation_layers && !supports_validation_layer()){
		log("Validation layers are enabled, but not supported.");
		return FAILURE;
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
	if(enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
	}
    VkInstance instance;
    if(vkCreateInstance(&create_info, NULL, &instance) != VK_SUCCESS) {
		log("Vulkan instance creation failed.");
        return FAILURE;
    }
	context->instance = instance;
  	log("Vulkan instance creation succesful!");
	 return SUCCESS;
}

int create_surface(VkContext *context){
	VkSurfaceKHR surface;
	if(glfwCreateWindowSurface(context->instance, context-> window, NULL, &surface) != VK_SUCCESS){
		log("Surface creation failed.");
		return FAILURE;
	}
	context->surface = surface;
	log("Surface creation succesful!");
	return SUCCESS;
}


uint32_t find_graphics_queue(VkPhysicalDevice physical_device){
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
	VkQueueFamilyProperties* queue_families_properties = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families_properties);

	for(uint32_t i = 0; i < queue_family_count; i++){
		if(queue_families_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
			free(queue_families_properties);
			return i;
		}
	}
	free(queue_families_properties);
	return UINT32_MAX;
}

uint32_t find_present_queue(VkSurfaceKHR surface, VkPhysicalDevice physical_device){
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, NULL);
	VkQueueFamilyProperties* queue_family_properties = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_family_properties);
	
	VkBool32 supports_presenting = false;
	for(uint32_t i = 0; i < queue_family_count; i++){
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_presenting);
		if(supports_presenting){
			free(queue_family_properties);
			return i;
		}
	}
	free(queue_family_properties);
	return UINT32_MAX;
}
// remove the quotations, its a macro, not a string :#
// returns false, so something going wrong 
bool supports_extensions(VkPhysicalDevice physical_device){
    // the presence of the presentation queue implicitly means it is supported!
    uint32_t supported_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &supported_extension_count, NULL);
	if(supported_extension_count == 0){
		return false;
	}
    VkExtensionProperties *supported_extension_properties_list = malloc(sizeof(VkExtensionProperties) * supported_extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &supported_extension_count, supported_extension_properties_list);

    for(int i = 0; i < extension_count; i++){
        bool extension_supported = false;
        for(int j = 0; j < supported_extension_count; j++){
            if(strcmp(extensions[i], supported_extension_properties_list[j].extensionName) == 0){
                extension_supported = true;
                break;
            }
        }
        if(!extension_supported){
            free(supported_extension_properties_list);
            return false;
        }
    }
    free(supported_extension_properties_list);
    return true;
}


int pick_physical_device(VkContext *context){
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, NULL);
	if(physical_device_count == 0){
		log("No physical devices available.");
		return FAILURE;
	}
	VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices);
	for(uint32_t i = 0; i < physical_device_count; i++){
		uint32_t graphics_queue_index = find_graphics_queue(physical_devices[i]);
		uint32_t present_queue_index = find_present_queue(context->surface, physical_devices[i]);
		if(graphics_queue_index != UINT32_MAX && present_queue_index != UINT32_MAX && supports_extensions(physical_devices[i])){
			context->physical_device = physical_devices[i];
			context->graphics_queue_index = graphics_queue_index;
			context->present_queue_index = present_queue_index;
			free(physical_devices);
			log("Physical device picked succesful!.");
			return SUCCESS;
		}
	}
	free(physical_devices);
	log("No suitable physical device found.");
	return FAILURE;	
}

int create_logical_device(VkContext *context){
	VkDeviceQueueCreateInfo queue_create_infos[2];

	VkDeviceQueueCreateInfo graphics_queue_create_info = {};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = context-> graphics_queue_index;
	graphics_queue_create_info.queueCount = 1;
	float queue_priority = 1.0f;
	graphics_queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos[0] = graphics_queue_create_info;
	
	VkDeviceQueueCreateInfo present_queue_create_info = {};
	present_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	present_queue_create_info.queueFamilyIndex = context->present_queue_index; 
	present_queue_create_info.queueCount = 1;	
	present_queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos[1] = present_queue_create_info;
	VkPhysicalDeviceFeatures device_features = {};
	
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.queueCreateInfoCount = 2;
	create_info.pEnabledFeatures = &device_features;
	// in modern vulkan, it is unnecesary, but usefull for old vulkan
	create_info.enabledExtensionCount = 0;
	if(enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
	}
	VkDevice logical_device;
	if(vkCreateDevice(context->physical_device, &create_info, NULL, &logical_device) != VK_SUCCESS){
		log("Logical device creation failed.");
		return FAILURE;
	}
	context->logical_device = logical_device;
	log("Logical device creation succesful!");
	return SUCCESS;
}

int set_graphics_queue(VkContext *context){
	VkQueue graphics_queue;
	vkGetDeviceQueue(context->logical_device, context-> graphics_queue_index, 0, &graphics_queue);
	if(graphics_queue == NULL){
		log("Setting the graphics queue failed.");
		return FAILURE;
	}
	context->graphics_queue = graphics_queue;
	log("Set the graphics queue succesfully!");
	return SUCCESS;
}

int set_present_queue(VkContext *context){
	VkQueue present_queue;
	vkGetDeviceQueue(context->logical_device, context->present_queue_index, 0, &present_queue);
	if(present_queue == NULL){
		log("Setting the present queue failed.");
		return FAILURE;
	}
	context->present_queue = present_queue;
	log("Set the present queue succesfully!");
	return SUCCESS;
}


void cleanup_vk_context(VkContext *context){
	if(context->logical_device != NULL){			
		vkDestroyDevice(context->logical_device, NULL);
		log("Destroyed the logical device.");
	}
	if(context->surface != NULL){	
		vkDestroySurfaceKHR(context->instance, context->surface, NULL);
		log("Destroyed the surface.");
	}
	if(context->instance != NULL){
		vkDestroyInstance(context->instance, NULL);
		log("Destroyed the vulkan instance.");
	}
	if(context->window != NULL){
    	glfwDestroyWindow(context->window);
		log("Destroyed the glfw window.");
	}
}


bool initialise_vulkan_context(VkContext *context){
	if(create_window(context) == FAILURE){
		return false;
	}
	if(create_vk_instance(context) == FAILURE){
		return false;
	}
	if(create_surface(context) == FAILURE){
		return false;
	}
	if(pick_physical_device(context) == FAILURE){
		return false;
	}
	if(create_logical_device(context) == FAILURE){
		return false;
	}
	if(set_graphics_queue(context) == FAILURE){
		return false;
	}
	if(set_present_queue(context) == FAILURE){
		return false;
	}
	return true;
}

int main(){
	glfwInit();

	VkContext context = {};
	if(!initialise_vulkan_context(&context)){
		cleanup_vk_context(&context);
		glfwTerminate();
		return 1;
	}
	while(!glfwWindowShouldClose(context.window)) {
        glfwPollEvents();
    }
	cleanup_vk_context(&context);
	
	glfwTerminate();
    return 0;
}


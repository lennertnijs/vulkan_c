#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aurora.h"
#include "aurora_internal_preferences.h"

typedef enum QueueType {
	GRAPHICS,
	PRESENT,
	TRANSFER,
	COMPUTE
} QueueType;

typedef struct {
	VkQueue queue;
	uint32_t index;
	QueueType type;
} Queue;

struct AuroraSession {
	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;
	int graphics_queue_count;
	Queue *graphics_queues;
	int present_queue_count;
	Queue *present_queues;
	int transfer_queue_count;
	Queue *transfer_queues;
	int compute_queue_count;
	Queue *compute_queues;
	VkSwapchainKHR swapchain;
	VkSurfaceFormatKHR image_format;
	VkExtent2D image_extent;
	VkPresentModeKHR present_mode;
	VkSurfaceTransformFlagBitsKHR transform;	
	uint32_t image_count;
	VkImage *images;
	VkImageView *image_views;
	VkPipelineLayout pipeline_layout;
	VkRenderPass render_pass;
	VkPipeline graphics_pipeline;
	int framebuffer_count;
	VkFramebuffer *framebuffers;
	VkCommandPool command_pool;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	int command_buffer_count;
	VkCommandBuffer *command_buffers;
	int image_available_semaphore_count;
	VkSemaphore *image_available_semaphores;
	int render_finished_semaphores_count;
	VkSemaphore *render_finished_semaphores;
	int in_flight_fence_count;
	VkFence *in_flight_fences;
};

void init_glfw_window(AuroraPreferences *preferences, AuroraSession *session){
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	if(preferences->allow_resize){
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}else{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	}
	if(preferences->width <= 0 || preferences->height <= 0){
		printf("The width/height of the window is negative or 0.");
	}
    GLFWwindow *window = glfwCreateWindow(preferences->width, preferences->height, "Vulkan", NULL, NULL);
	assert(window != NULL);
	session->window = window;
}


bool supports_requested_validation_layers(AuroraPreferences *preferences){
    uint32_t supported_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);

    VkLayerProperties *supported_layers = malloc(sizeof(VkLayerProperties) * supported_layer_count);
	assert(supported_layers != NULL);
    vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers);
    
    for(int i = 0; i < preferences->validation_layer_count; i++){
		bool layer_supported = false;
		for(uint32_t j = 0; j < supported_layer_count; j++){
			if(strcmp(preferences->validation_layers[i], supported_layers[j].layerName) == 0){
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

void init_vk_instance(AuroraPreferences *preferences, AuroraSession *session) {
	if(preferences->enable_validation_layers && !supports_requested_validation_layers(preferences)){
		printf("The requested validation layers are not supported.");
		assert(NULL);
	}
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = preferences->application_name;
    app_info.applicationVersion = preferences->application_version;
    app_info.pEngineName = preferences->engine_name;
    app_info.engineVersion = preferences->engine_version;
    app_info.apiVersion = preferences->api_version;
	
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
	if(preferences->enable_validation_layers){
		create_info.enabledLayerCount = preferences->validation_layer_count;
		create_info.ppEnabledLayerNames = preferences->validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}
	VkInstance instance;
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
	assert(result == VK_SUCCESS);
	session->instance = instance;
}


void init_surface(AuroraPreferences *preferences, AuroraSession *session){
	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(session->instance, session->window, NULL, &surface);
	assert(result == VK_SUCCESS);
	session->surface = surface;
}

bool has_graphics_queue(VkPhysicalDevice physical_device){
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
	VkQueueFamilyProperties *properties = malloc(sizeof(VkQueueFamilyProperties));
	assert(properties != NULL);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties);
	
	bool supports_graphics = false;
	for(uint32_t i = 0; i < count; i++){
		if((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0){
			supports_graphics = true;
			break;
		}
	}
	free(properties);
	return supports_graphics;
}

bool has_present_queue(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
	VkQueueFamilyProperties *properties = malloc(sizeof(VkQueueFamilyProperties) * count);
	assert(properties != NULL);
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, properties);

	VkBool32 supports_presenting;
	for(uint32_t i = 0; i < count; i++){	
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_presenting);
		if(supports_presenting){
			break;
		}
	}
	free(properties);
	return supports_presenting;
}

bool supports_extensions(VkPhysicalDevice physical_device, const char** extensions, int amount){
    uint32_t count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, NULL);
	if(count == 0){
		return false;
	}
    VkExtensionProperties *supported_extensions = malloc(sizeof(VkExtensionProperties) * count);
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &count, supported_extensions);
	for(int i = 0; i < amount; i++){
		bool supported = false;
		for(uint32_t j = 0; j < count; j++){
			if(strcmp(extensions[i], supported_extensions[j].extensionName) == 0){
				supported = true;
				break;
			}
		}
		if(!supported){
			free(supported_extensions);
			return false;
		}
	}
	free(supported_extensions);
	return true;
}


bool supports_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
	uint32_t count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, NULL);
	return count > 0; 
}

void init_physical_device(AuroraPreferences *preferences, AuroraSession *session){
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(session->instance, &count, NULL);
	VkPhysicalDevice *physical_devices = malloc(sizeof(VkPhysicalDevice) * count);
	assert(physical_devices);
	vkEnumeratePhysicalDevices(session->instance, &count, physical_devices);
	
	bool selected = false;
	for(uint32_t i = 0; i < count; i++){
		if(!has_graphics_queue(physical_devices[i])){
			continue;
		}
		if(!has_present_queue(physical_devices[i], session->surface)){
			continue;
		}
		if(!supports_extensions(physical_devices[i], preferences->extensions, preferences->extension_count)){
			continue;
		}
		if(!supports_surface_format(physical_devices[i], session->surface)){
			continue;
		}
		session->physical_device = physical_devices[i];
		selected = true;
	}
	free(physical_devices);
	if(!selected){
		printf("No suitable physical device was selected.");
	}
}

AuroraSession *aurora_session_create(AuroraPreferences* preferences){
	if(preferences == NULL){
		printf("Aurora preferences is NULL.");
	}
	AuroraSession *session = malloc(sizeof(AuroraSession));
	glfwInit();
	init_glfw_window(preferences, session);
	init_vk_instance(preferences, session);
	init_surface(preferences, session);
	init_physical_device(preferences, session);
	return session;
}

void aurora_session_destroy(AuroraSession *session){
	vkDestroySurfaceKHR(session->instance, session->surface, NULL);
	vkDestroyInstance(session->instance, NULL);
	glfwDestroyWindow(session->window);
	glfwTerminate();
	free(session);
}

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aurora.h"
#include "aurora_internal_config.h"

typedef enum QueueType {
	NONE = 0,
	GRAPHICS = 1 << 0,
	PRESENT = 1 << 1,
	COMPUTE = 1 << 2,
	TRANSFER = 1 << 3
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
	int queue_count;
	Queue *queues;
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

void init_glfw_window(AuroraConfig *config, AuroraSession *session){
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	if(config->allow_resize){
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	}else{
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	}
	if(config->width <= 0 || config->height <= 0){
		printf("The width/height of the window is negative or 0.");
	}
    GLFWwindow *window = glfwCreateWindow(config->width, config->height, "Vulkan", NULL, NULL);
	assert(window != NULL);
	session->window = window;
}


bool supports_requested_validation_layers(AuroraConfig *config){
    uint32_t supported_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);

    VkLayerProperties *supported_layers = malloc(sizeof(VkLayerProperties) * supported_layer_count);
	assert(supported_layers != NULL);
    vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers);
    
    for(int i = 0; i < config->validation_layer_count; i++){
		bool layer_supported = false;
		for(uint32_t j = 0; j < supported_layer_count; j++){
			if(strcmp(config->validation_layers[i], supported_layers[j].layerName) == 0){
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

void init_vk_instance(AuroraConfig *config, AuroraSession *session) {
	if(config->enable_validation_layers && !supports_requested_validation_layers(config)){
		printf("The requested validation layers are not supported.");
		assert(NULL);
	}
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = config->application_name;
    app_info.applicationVersion = config->application_version;
    app_info.pEngineName = config->engine_name;
    app_info.engineVersion = config->engine_version;
    app_info.apiVersion = config->api_version;
	
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;
	if(config->enable_validation_layers){
		create_info.enabledLayerCount = config->validation_layer_count;
		create_info.ppEnabledLayerNames = config->validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}
	VkInstance instance;
    VkResult result = vkCreateInstance(&create_info, NULL, &instance);
	assert(result == VK_SUCCESS);
	session->instance = instance;
}


void init_surface(AuroraConfig *config, AuroraSession *session){
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


void init_physical_device(AuroraConfig *config, AuroraSession *session){
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
		if(!supports_extensions(physical_devices[i], config->extensions, config->extension_count)){
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

void get_queue_indices(AuroraConfig *config, AuroraSession *session){
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(session->physical_device, &count, NULL);
	if(count == 0){
		return;
	}
	VkQueueFamilyProperties *properties = malloc(sizeof(VkQueueFamilyProperties) * count);
	vkGetPhysicalDeviceQueueFamilyProperties(session->physical_device, &count, properties);
	int max = config->graphics_queue_count + config->present_queue_count + config->compute_queue_count + config->transfer_queue_count;
	int graphics_count = 0;
	int present_count = 0;
	int compute_count = 0;
	int transfer_count = 0;
	int queue_count = 0;
	QueueType *queue_types = malloc(sizeof(QueueType) * max);	
	uint32_t *queue_indices = malloc(sizeof(uint32_t) * max);
	for(uint32_t i = 0; i < count; i++){
		QueueType queue_type = NONE;
		VkBool32 supports_presenting = false;
		if((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && graphics_count < config->graphics_queue_count){
			queue_type |= GRAPHICS;			
			graphics_count++;
		}
		if((properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0 && compute_count < config->compute_queue_count){
			queue_type |= COMPUTE;
			compute_count++;
		}	
		if((properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 && transfer_count < config->transfer_queue_count){
			queue_type |= TRANSFER;
			transfer_count++;
		}	
		vkGetPhysicalDeviceSurfaceSupportKHR(session->physical_device, i, session->surface, &supports_presenting);
		if(supports_presenting && present_count < config->present_queue_count){	
			queue_type |= PRESENT;	
			present_count++;
		}
		if(queue_type != NONE){
			queue_types[queue_count] = queue_type;
			queue_indices[queue_count] = i;
			queue_count++;
		}
	}
	free(properties);
	Queue *queues = malloc(sizeof(Queue) * queue_count);
	for(int i = 0; i < queue_count; i++){
		queues[i] = (Queue){
			.index = queue_indices[i],
			.type = queue_types[i]
		};
	}
	session->queues = queues;
	session->queue_count = queue_count;
	free(queue_indices);
	free(queue_types);
	if(graphics_count != config->graphics_queue_count){
		printf("Insufficient graphics queues found.");
		exit(1);
	}
	if(present_count != config->present_queue_count){
		printf("Insufficient present queues found.");
		exit(1);
	}
	if(compute_count != config->compute_queue_count){
		printf("Insufficient compute queues found.");
		exit(1);
	}
	if(transfer_count != config->transfer_queue_count){
		printf("Insufficient transfer queues found.");
		exit(1);
	}
}

void init_logical_device(AuroraConfig *config, AuroraSession *session){
	get_queue_indices(config, session);

	VkDeviceQueueCreateInfo *infos = malloc(sizeof(VkDeviceQueueCreateInfo) * session->queue_count);
	for(int i = 0; i < session->queue_count; i++){
		float priority = 1.0f;
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = session->queues[i].index;
		queue_create_info.queueCount = 1; 		
		queue_create_info.pQueuePriorities = &priority;
		infos[i] = queue_create_info;
	}	
	
	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = infos;
	create_info.queueCreateInfoCount = session->queue_count;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = config->extension_count;
	create_info.ppEnabledExtensionNames = config->extensions;
	if(config->enable_validation_layers){
		create_info.enabledLayerCount = config->validation_layer_count;
		create_info.ppEnabledLayerNames = config->validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}
	VkResult result = vkCreateDevice(session->physical_device, &create_info, NULL, &session->device);
	assert(result == VK_SUCCESS);
	
	for(int i = 0; i < session->queue_count; i++){
		vkGetDeviceQueue(session->device, session->queues[i].index, 0, &session->queues[i].queue);
	}
}

AuroraSession *aurora_session_create(AuroraConfig *config){
	if(config == NULL){
		printf("Aurora config is NULL.");
	}
	AuroraSession *session = malloc(sizeof(AuroraSession));
	glfwInit();
	init_glfw_window(config, session);
	init_vk_instance(config, session);
	init_surface(config, session);
	init_physical_device(config, session);
	init_logical_device(config, session);
	return session;
}

void aurora_session_destroy(AuroraSession *session){
	vkDestroyDevice(session->device, NULL);
	vkDestroySurfaceKHR(session->instance, session->surface, NULL);
	vkDestroyInstance(session->instance, NULL);
	glfwDestroyWindow(session->window);
	glfwTerminate();
	free(session);
}

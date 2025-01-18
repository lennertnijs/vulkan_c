#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "math.c"
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


void init_surface(AuroraSession *session){
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

VkPresentModeKHR translate_present_mode(PresentMode present_mode){
	switch(present_mode){
		case IMMEDIATE: 
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		case MAILBOX:
			return VK_PRESENT_MODE_MAILBOX_KHR;
		case FIFO:
			return VK_PRESENT_MODE_FIFO_KHR;
		case FIFO_RELAXED:
			return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
		default:
			exit(1);
	}
}

uint32_t find_queue_type(AuroraSession *session, QueueType type){
	for(int i = 0; i < session->queue_count; i++){
		if((session->queues[i].type & type) != 0){
			return i;
		}
	}
	exit(1);
}

void init_swapchain(AuroraConfig *config, AuroraSession *session){
	VkSurfaceCapabilitiesKHR capabilities= {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(session->physical_device, session->surface, &capabilities);

	if(capabilities.maxImageCount != 0 && ((uint32_t)config->image_count) > capabilities.maxImageCount){
		session->image_count = capabilities.maxImageCount;
	}else if(((uint32_t)config->image_count) < capabilities.minImageCount){
		session->image_count = capabilities.minImageCount;
	}else{
		session->image_count = config->image_count;
	}
	if(capabilities.currentExtent.width != UINT32_MAX){
		session->image_extent = capabilities.currentExtent;
	}else{
		int width, height;
		glfwGetFramebufferSize(session->window, &width, &height);
		session->image_extent = (VkExtent2D){
			.width = clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			.height = clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(session->physical_device, session->surface, &format_count, NULL);
	if(format_count == 0){
		printf("No format counts supported.");
		exit(1);
	}
	VkSurfaceFormatKHR *surface_formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(session->physical_device, session->surface, &format_count, surface_formats);
	session->image_format = surface_formats[0];
	for(uint32_t i = 0; i < format_count; i++){
		if(surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			session->image_format = surface_formats[i];
			break;
		}
	}
	free(surface_formats);
	
	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(session->physical_device, session->surface, &present_mode_count, NULL);
	if(present_mode_count == 0){
		printf("No Support modes supported.");
		exit(1);
	}
	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(session->physical_device, session->surface, &present_mode_count, present_modes);
	VkPresentModeKHR required_mode = translate_present_mode(config->present_mode);
	for(uint32_t i = 0; i < present_mode_count; i++){
		if(present_modes[i] == required_mode){
			session->present_mode = required_mode;
		}
	}
	
	session->transform = capabilities.currentTransform;
	
	VkSwapchainCreateInfoKHR create_info = {}; 
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = session->surface;
	create_info.minImageCount = session->image_count;
	create_info.imageFormat = session->image_format.format;
	create_info.imageColorSpace = session->image_format.colorSpace;
	create_info.imageExtent = session->image_extent;
	create_info.imageArrayLayers  = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t indices[2];
	indices[0] = find_queue_type(session, GRAPHICS);
	indices[1] = find_queue_type(session, PRESENT); 
	if(indices[0] != indices[1]){
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = indices;
	}else{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = NULL;
	}
	create_info.preTransform = session->transform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = session->present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	VkResult result = vkCreateSwapchainKHR(session->device, &create_info, NULL, &session->swapchain);
	assert(result == VK_SUCCESS);
}

void init_images(AuroraSession *session){
	uint32_t count = 0;
	vkGetSwapchainImagesKHR(session->device, session->swapchain, &count, NULL);
	if(count == 0){
		printf("No images in the swapchain.");
		exit(1);
	}
	VkImage *images = malloc(sizeof(VkImage) * count);
	vkGetSwapchainImagesKHR(session->device, session->swapchain, &count, images);
	session->images = images;
}

void init_image_views(AuroraSession *session){
	int image_count = session->image_count;
	session->image_views = malloc(sizeof(VkImageView) * image_count);
	for(int i = 0; i < image_count; i++){
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = session->images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = session->image_format.format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		VkResult err = vkCreateImageView(session->device, &create_info, NULL, &session->image_views[i]);
		if(err){
			printf("Creation of the image view failed.");
			exit(1);
		}
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
	init_surface(session);
	init_physical_device(config, session);
	init_logical_device(config, session);
	init_swapchain(config, session);
	init_images(session);
	init_image_views(session);
	return session;
}

void aurora_session_destroy(AuroraSession *session){
	for(uint32_t i = 0; i < session->image_count; i++){
		vkDestroyImageView(session->device, session->image_views[i], NULL);
	}
	free(session->image_views);
	free(session->images);
	vkDestroySwapchainKHR(session->device, session->swapchain, NULL);
	vkDestroyDevice(session->device, NULL);
	vkDestroySurfaceKHR(session->instance, session->surface, NULL);
	vkDestroyInstance(session->instance, NULL);
	glfwDestroyWindow(session->window);
	glfwTerminate();
	free(session);
}

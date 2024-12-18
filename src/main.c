#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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
	VkSwapchainKHR swapchain;
	VkImage *swapchain_images;
	uint32_t image_count;
	VkFormat swapchain_image_format;
	VkExtent2D swapchain_extent;
	VkImageView *swapchain_image_views;
} VkContext;

int create_window(VkContext* context) {
	assert(context != NULL);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    context->window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	if(context->window == NULL){
		return FAILURE;
	}
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
	assert(context != NULL);
	if(enable_validation_layers && !supports_validation_layer()){
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
    if(vkCreateInstance(&create_info, NULL, &context->instance) != VK_SUCCESS) {
        return FAILURE;
    }
	return SUCCESS;
}

int create_surface(VkContext *context){
	assert(context != NULL);
	if(glfwCreateWindowSurface(context->instance, context->window, NULL, &context->surface) != VK_SUCCESS){
		return FAILURE;
	}
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

// there was a third thing here for the swapchain requirements. no clue what, though. see around p75

VkSurfaceFormatKHR select_swapchain_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
	VkSurfaceFormatKHR selected_format;

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
	if(format_count == 0){
		log("No formats are supported.");
		selected_format.format = VK_FORMAT_UNDEFINED;
		return selected_format;
	}
	VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, formats);

	selected_format = formats[0];
	for(uint32_t i = 0; i < format_count; i++){
		if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			selected_format = formats[i];
			break;
		}
	}
	free(formats);
	return selected_format;
}

VkPresentModeKHR select_swapchain_present_mode(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
	VkPresentModeKHR selected_present_mode;

	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
	if(present_mode_count == 0){
		log("No present modes are supported.");
		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, present_modes);
	
	for(uint32_t i = 0; i < present_mode_count; i++){
		if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
			selected_present_mode = present_modes[i];
			break;
		}
	}
	free(present_modes);
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D select_swapchain_extent(VkPhysicalDevice physical_device, VkSurfaceKHR surface, GLFWwindow *window){
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
	
	if(capabilities.currentExtent.width != UINT32_MAX){
		return capabilities.currentExtent;
	}else{
		uint32_t width, height;
		glfwGetFramebufferSize(window, &width, &height);
		
		width = width < capabilities.minImageExtent.width ? capabilities.minImageExtent.width : width;
		width = width > capabilities.maxImageExtent.width ? capabilities.maxImageExtent.width : width;
		height = height < capabilities.minImageExtent.height ? capabilities.minImageExtent.height : height;
		height = height > capabilities.maxImageExtent.height ? capabilities.maxImageExtent.height : height;
		VkExtent2D extent = {};
		extent.width = width;
		extent.height = height;
		return extent;	
	}
}

int pick_physical_device(VkContext *context){
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, NULL);
	if(physical_device_count == 0){
		return FAILURE;
	}
	VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices);
	
	for(uint32_t i = 0; i < physical_device_count; i++){
		uint32_t graphics_queue_index = find_graphics_queue(physical_devices[i]);
		uint32_t present_queue_index = find_present_queue(context->surface, physical_devices[i]);
		if(graphics_queue_index == UINT32_MAX || present_queue_index == UINT32_MAX){	
			return FAILURE;
		}
		if(!supports_extensions(physical_devices[i])){
			return FAILURE;
		}
		if(select_swapchain_surface_format(physical_devices[i], context->surface).format == VK_FORMAT_UNDEFINED){
			return FAILURE;
		}
		context->physical_device = physical_devices[i];
		context->graphics_queue_index = graphics_queue_index;
		context->present_queue_index = present_queue_index;
		free(physical_devices);
		return SUCCESS;
		}
	free(physical_devices);
	return FAILURE;
}

int create_logical_device(VkContext *context){
	VkDeviceQueueCreateInfo queue_create_infos[2];
	int actual_queue_count = context->graphics_queue_index != context->graphics_queue_index ? 2 : 1;
	VkDeviceQueueCreateInfo graphics_queue_create_info = {};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = context-> graphics_queue_index;
	graphics_queue_create_info.queueCount = 1;
	float queue_priority = 1.0f;
	graphics_queue_create_info.pQueuePriorities = &queue_priority;
	queue_create_infos[0] = graphics_queue_create_info;
	if(context->graphics_queue_index != context->present_queue_index){
		VkDeviceQueueCreateInfo present_queue_create_info = {};
		present_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		present_queue_create_info.queueFamilyIndex = context->present_queue_index; 
		present_queue_create_info.queueCount = 1;	
		present_queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos[1] = present_queue_create_info;
	}	
	
	VkPhysicalDeviceFeatures device_features = {};
	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_create_infos;
	create_info.queueCreateInfoCount = actual_queue_count;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extensions;
	if(enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
	}
	if(vkCreateDevice(context->physical_device, &create_info, NULL, &context->logical_device) != VK_SUCCESS){
		return FAILURE;
	}
	return SUCCESS;
}

int set_graphics_queue(VkContext *context){
	assert(context != NULL);
	vkGetDeviceQueue(context->logical_device, context->graphics_queue_index, 0, &context->graphics_queue);
	if(context->graphics_queue == NULL){
		return FAILURE;
	}
	return SUCCESS;
}

int set_present_queue(VkContext *context){
	assert(context != NULL);
	vkGetDeviceQueue(context->logical_device, context->present_queue_index, 0, &context->present_queue);
	if(context->present_queue == NULL){
		return FAILURE;
	}
	return SUCCESS;
}

int create_swapchain(VkContext *context){	
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &capabilities);
	VkSurfaceFormatKHR format = select_swapchain_surface_format(context->physical_device, context->surface);
	VkPresentModeKHR present_mode = select_swapchain_present_mode(context->physical_device, context->surface);
	uint32_t image_count = capabilities.minImageCount + 1;
	VkExtent2D extent = select_swapchain_extent(context->physical_device, context->surface, context->window);
	if(image_count > capabilities.maxImageCount && capabilities.maxImageCount > 0){
		image_count = capabilities.maxImageCount;
	}
	
	VkSwapchainCreateInfoKHR create_info = {}; 
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = context->surface;
	create_info.minImageCount = image_count;
	create_info.imageFormat = format.format;
	create_info.imageColorSpace = format.colorSpace;
	create_info.imageExtent = extent;
	create_info.imageArrayLayers  = 1;
	create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t indices[2];
	indices[0] = context->graphics_queue_index;
	indices[1] = context->present_queue_index; 
	if(context->graphics_queue_index != context->present_queue_index){
		create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		create_info.queueFamilyIndexCount = 2;
		create_info.pQueueFamilyIndices = indices;
	}else{
		create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices = NULL;
	}
	create_info.preTransform = capabilities.currentTransform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	if(vkCreateSwapchainKHR(context->logical_device, &create_info, NULL, &context->swapchain) != VK_SUCCESS){
		return FAILURE;
	}
	return SUCCESS;
}

int create_swapchain_images(VkContext *context){
	assert(context != NULL);
	uint32_t image_count = 0;
	vkGetSwapchainImagesKHR(context->logical_device, context->swapchain, &image_count, NULL);
	context->swapchain_images = malloc(sizeof(VkImage) * image_count);
	vkGetSwapchainImagesKHR(context->logical_device, context->swapchain, &image_count, context->swapchain_images);
	context->image_count = image_count;
	return SUCCESS;	
}

int set_swapchain_format(VkContext *context){	
	context->swapchain_image_format = select_swapchain_surface_format(context->physical_device, context->surface).format;
	return SUCCESS;
}

int set_swapchain_extent(VkContext *context){
	context->swapchain_extent = select_swapchain_extent(context->physical_device, context->surface, context->window);
	return SUCCESS;
}

int create_swapchain_image_views(VkContext *context){
	context->swapchain_image_views = malloc(sizeof(VkImageView) * context->image_count);
	for(uint32_t i = 0; i < context->image_count; i++){
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = context->swapchain_images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = context->swapchain_image_format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		if(vkCreateImageView(context->logical_device, &create_info, NULL, &context->swapchain_image_views[i]) != VK_SUCCESS){
			return FAILURE;
		}
	}
	return SUCCESS;	
}

void cleanup_vk_context(VkContext *context){
	if(context->swapchain_image_views != NULL){
		for(uint32_t i = 0; i < context->image_count; i++){
			vkDestroyImageView(context->logical_device, context->swapchain_image_views[i], NULL);
		}
		free(context->swapchain_image_views);
	}	
	if(context->swapchain_images != NULL){
		free(context->swapchain_images);
	}
	if(context->swapchain != NULL){
		vkDestroySwapchainKHR(context->logical_device, context->swapchain, NULL);
	}
	if(context->logical_device != NULL){			
		vkDestroyDevice(context->logical_device, NULL);
	}
	if(context->surface != NULL){	
		vkDestroySurfaceKHR(context->instance, context->surface, NULL);
	}
	if(context->instance != NULL){
		vkDestroyInstance(context->instance, NULL);
	}
	if(context->window != NULL){
    	glfwDestroyWindow(context->window);
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
	if(create_swapchain(context) == FAILURE){
		return false;
	}
	if(create_swapchain_images(context) == FAILURE){
		return false;
	}
	if(set_swapchain_format(context) == FAILURE){
		return false;
	}
	if(set_swapchain_extent(context) == FAILURE){
		return false;
	}
	if(create_swapchain_image_views(context) == FAILURE){
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


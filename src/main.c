	#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "math.c"

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
const char* extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
const int extension_count = 1;
typedef struct {
	uint32_t image_count;
	VkSurfaceFormatKHR image_format;
	VkExtent2D image_extent;
	VkPresentModeKHR present_mode;
	VkSurfaceTransformFlagBitsKHR transform;
} SwapchainInfo;

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
	SwapchainInfo swapchain_info;
	VkImage *images;
	VkImageView *image_views;
	VkPipelineLayout pipeline_layout;
	VkRenderPass render_pass;
	VkPipeline graphics_pipeline;
	VkFramebuffer *frame_buffers;
	VkCommandPool command_pool;
	VkCommandBuffer command_buffer;
	VkSemaphore image_available_semaphore;
	VkSemaphore render_finished_semaphore;
	VkFence in_flight_fence;
} VkContext;

void create_window(VkContext* context) {
	assert(context != NULL);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // dont use openGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    context->window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	assert(context->window != NULL);
}

bool supports_validation_layer(){
    uint32_t supported_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);
    VkLayerProperties* supported_layers_list = malloc(sizeof(VkLayerProperties) * supported_layer_count);
	assert(supported_layers_list != NULL);
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

void create_vk_instance(VkContext *context) {
	assert(context != NULL);
	if(enable_validation_layers && !supports_validation_layer()){
		return;
	}
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
    VkResult result = vkCreateInstance(&create_info, NULL, &context->instance);
	assert(result == VK_SUCCESS);
}

void create_surface(VkContext *context){
	assert(context != NULL);
	VkResult result = glfwCreateWindowSurface(context->instance, context->window, NULL, &context->surface);
	assert(result == VK_SUCCESS);
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
	assert(physical_device != NULL);
    uint32_t supported_extension_count = 0;
	vkEnumerateDeviceExtensionProperties(physical_device, NULL, &supported_extension_count, NULL);
	if(supported_extension_count == 0){
		return false;
	}
    VkExtensionProperties *supported_extensions = malloc(sizeof(VkExtensionProperties) * supported_extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &supported_extension_count, supported_extensions);

    for(int i = 0; i < extension_count; i++){
        bool extension_supported = false;
        for(uint32_t j = 0; j < supported_extension_count; j++){
            if(strcmp(extensions[i], supported_extensions[j].extensionName) == 0){
                extension_supported = true;
                break;
            }
        }
        if(!extension_supported){
            free(supported_extensions);
            return false;
        }
    }
    free(supported_extensions);
    return true;
}

bool supports_surface_format(VkPhysicalDevice physical_device, VkSurfaceKHR surface){
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, NULL);
	return format_count > 0; 
}

void pick_physical_device(VkContext *context){
	assert(context != NULL);
	uint32_t physical_device_count = 0;
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, NULL);
	assert(physical_device_count);
	VkPhysicalDevice* physical_devices = malloc(sizeof(VkPhysicalDevice) * physical_device_count);
	vkEnumeratePhysicalDevices(context->instance, &physical_device_count, physical_devices);
	
	for(uint32_t i = 0; i < physical_device_count; i++){
		uint32_t graphics_queue_index = find_graphics_queue(physical_devices[i]);
		uint32_t present_queue_index = find_present_queue(context->surface, physical_devices[i]);
		if(graphics_queue_index == UINT32_MAX || present_queue_index == UINT32_MAX 
			|| !supports_extensions(physical_devices[i])
			|| !supports_surface_format(physical_devices[i], context->surface)){	
			continue;
		}
		context->physical_device = physical_devices[i];
		break;
		}
	free(physical_devices);
	assert(context->physical_device != NULL);
}

void create_logical_device(VkContext *context){	
	assert(context != NULL);
	context->graphics_queue_index = find_graphics_queue(context->physical_device);
	context->present_queue_index = find_present_queue(context->surface, context->physical_device);

	VkDeviceQueueCreateInfo queue_create_infos[2];
	int queue_count = context->graphics_queue_index != context->present_queue_index ? 2 : 1;
	VkDeviceQueueCreateInfo graphics_queue_create_info = {};
	graphics_queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	graphics_queue_create_info.queueFamilyIndex = context->graphics_queue_index;
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
	create_info.queueCreateInfoCount = queue_count;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extensions;
	if(enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
	}
	VkResult result = vkCreateDevice(context->physical_device, &create_info, NULL, &context->logical_device);
	assert(result == VK_SUCCESS);

	vkGetDeviceQueue(context->logical_device, context->graphics_queue_index, 0, &context->graphics_queue);
	assert(context->graphics_queue != NULL);

	vkGetDeviceQueue(context->logical_device, context->present_queue_index, 0, &context->present_queue);
	assert(context->present_queue != NULL);
}

void query_swapchain_info(VkContext *context, SwapchainInfo *info){
	assert(context != NULL);
	assert(info != NULL);
	
	VkSurfaceCapabilitiesKHR capabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physical_device, context->surface, &capabilities);

	info->image_count = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount != 0 && info->image_count > capabilities.maxImageCount){
		info->image_count = capabilities.maxImageCount;
	}	
	
	if(capabilities.currentExtent.width != UINT32_MAX){
		info->image_extent = capabilities.currentExtent;
	}else{
		int width, height;
		glfwGetFramebufferSize(context->window, &width, &height);
		info->image_extent = (VkExtent2D) {
			.width = clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			.height = clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)	
		};	
	}
	
	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &format_count, NULL);
	assert(format_count > 0);
	VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
	assert(formats != NULL);
	vkGetPhysicalDeviceSurfaceFormatsKHR(context->physical_device, context->surface, &format_count, formats);
	info->image_format = formats[0];
	for(uint32_t i = 0; i < format_count; i++){
		if(formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			info->image_format = formats[i];
			break;
		}
	}
	free(formats);
	
	uint32_t present_mode_count = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &present_mode_count, NULL);
	assert(present_mode_count > 0);
	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	assert(present_modes != NULL);
	vkGetPhysicalDeviceSurfacePresentModesKHR(context->physical_device, context->surface, &present_mode_count, present_modes);
	info->present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for(uint32_t i = 0; i < present_mode_count; i++){
		if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR){
			info->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	free(present_modes);

	info->transform = capabilities.currentTransform;
}

void create_swapchain(VkContext *context){
	assert(context != NULL);
	SwapchainInfo swapchain_info; 
	query_swapchain_info(context, &swapchain_info);	
	
	VkSwapchainCreateInfoKHR create_info = {}; 
	create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	create_info.surface = context->surface;
	create_info.minImageCount = swapchain_info.image_count;
	create_info.imageFormat = swapchain_info.image_format.format;
	create_info.imageColorSpace = swapchain_info.image_format.colorSpace;
	create_info.imageExtent = swapchain_info.image_extent;
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
	create_info.preTransform = swapchain_info.transform;
	create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	create_info.presentMode = swapchain_info.present_mode;
	create_info.clipped = VK_TRUE;
	create_info.oldSwapchain = VK_NULL_HANDLE;
	VkResult result = vkCreateSwapchainKHR(context->logical_device, &create_info, NULL, &context->swapchain);
	assert(result == VK_SUCCESS);	
	context->swapchain_info = swapchain_info;
}

void create_images(VkContext *context){
	assert(context != NULL);
	uint32_t image_count = 0;
	vkGetSwapchainImagesKHR(context->logical_device, context->swapchain, &image_count, NULL);
	context->images = malloc(sizeof(VkImage) * image_count);
	vkGetSwapchainImagesKHR(context->logical_device, context->swapchain, &image_count, context->images);	
}


void create_image_views(VkContext *context){
	context->image_views = malloc(sizeof(VkImageView) * context->swapchain_info.image_count);
	for(uint32_t i = 0; i < context->swapchain_info.image_count; i++){
		VkImageViewCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.image = context->images[i];
		create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		create_info.format = context->swapchain_info.image_format.format;
		create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		create_info.subresourceRange.baseMipLevel = 0;
		create_info.subresourceRange.levelCount = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount = 1;
		VkResult result = vkCreateImageView(context->logical_device, &create_info, NULL, &context->image_views[i]);
		assert(result == VK_SUCCESS);
	}
}

size_t fetch_file_size(char* file_name){
	FILE *file = fopen(file_name, "rb");
	fseek(file, 0, SEEK_END);
	size_t length = (size_t)ftell(file);
	fclose(file);
	return length;
}

char* read_file(char* file_name, size_t amount){
	assert(file_name != NULL);
	FILE *file = fopen(file_name, "rb");	
	char* buffer = (char*)malloc(amount);
	size_t read_size = fread(buffer, 1, amount, file);
	fclose(file);
	return buffer;
}

VkShaderModule create_shader_module(VkContext *context, char* code, size_t length){
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = length;
	create_info.pCode = (uint32_t*)code; // align the char* to uint32_t (so, 4x the space from 1 byte to 4 bytes)
	VkShaderModule shader_module;
	if(vkCreateShaderModule(context->logical_device, &create_info, NULL, &shader_module) != VK_SUCCESS){
		return NULL;
	}
	return shader_module;
}

void create_render_pass(VkContext *context){
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = context->swapchain_info.image_format.format;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	
	VkAttachmentReference color_attachment_reference = {};
	color_attachment_reference.attachment = 0; // fragment shader index location = 0
	color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_reference;


	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;	
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo render_pass_create_info = {};
	render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_create_info.attachmentCount = 1;
	render_pass_create_info.pAttachments = &color_attachment;
	render_pass_create_info.subpassCount = 1;
	render_pass_create_info.pSubpasses = &subpass;
	render_pass_create_info.dependencyCount = 1;
	render_pass_create_info.pDependencies = &dependency;
	VkResult result = vkCreateRenderPass(context->logical_device, &render_pass_create_info, NULL, &context->render_pass);
	assert(result == VK_SUCCESS);
}

void create_graphics_pipeline(VkContext *context){
	size_t vert_shader_length = fetch_file_size("src/vert.spv");
	char *vert_shader_code = read_file("src/vert.spv", vert_shader_length);
	size_t frag_shader_length = fetch_file_size("src/frag.spv");
	char *frag_shader_code = read_file("src/frag.spv", frag_shader_length);
	VkShaderModule vertex_shader_module = create_shader_module(context, vert_shader_code, vert_shader_length);
	VkShaderModule fragment_shader_module = create_shader_module(context, frag_shader_code, frag_shader_length);
	
	VkPipelineShaderStageCreateInfo vertex_shader_create_info = {};
	vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_create_info.module = vertex_shader_module;
	vertex_shader_create_info.pName = "main";
	
	VkPipelineShaderStageCreateInfo fragment_shader_create_info = {};
	fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_create_info.module = fragment_shader_module;
	fragment_shader_create_info.pName = "main";
	
	VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_shader_create_info, fragment_shader_create_info};	

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 0;
	vertex_input_info.pVertexBindingDescriptions = NULL;
	vertex_input_info.vertexAttributeDescriptionCount = 0;
	vertex_input_info.pVertexAttributeDescriptions = NULL;
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
	
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float) context->swapchain_info.image_extent.width;
	viewport.height = (float) context->swapchain_info.image_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	VkOffset2D offset = {0, 0};
	scissor.offset = offset;
	scissor.extent = context->swapchain_info.image_extent;
	
	VkPipelineViewportStateCreateInfo viewport_state_create_info = {};
	viewport_state_create_info.sType = 	VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;	
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {};
	rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_create_info.depthClampEnable = VK_FALSE;
	rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_create_info.lineWidth = 1.0f;
	rasterizer_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer_create_info.depthBiasEnable = VK_FALSE;
	rasterizer_create_info.depthBiasConstantFactor = 0.0f;
	rasterizer_create_info.depthBiasClamp = 0.0f;
	rasterizer_create_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
	multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_create_info.sampleShadingEnable = VK_FALSE;
	multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_create_info.minSampleShading = 1.0f;
	multisample_create_info.pSampleMask = NULL;
	multisample_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment_create_info = {};
	color_blend_attachment_create_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_create_info.blendEnable = VK_FALSE;
	color_blend_attachment_create_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_create_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_create_info.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_create_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_create_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_create_info.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
	color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend_create_info.logicOpEnable = VK_FALSE;
	color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_create_info.attachmentCount = 1;
	color_blend_create_info.pAttachments = &color_blend_attachment_create_info;
	color_blend_create_info.blendConstants[0] = 0.0f;
	color_blend_create_info.blendConstants[1] = 0.0f;
	color_blend_create_info.blendConstants[2] = 0.0f;
	color_blend_create_info.blendConstants[3] = 0.0f;

	VkDynamicState states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = 2;
	dynamic_state_create_info.pDynamicStates = &states[0];

	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 0;
	pipeline_layout_create_info.pSetLayouts = NULL;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = NULL;
	VkResult result = vkCreatePipelineLayout(context->logical_device, &pipeline_layout_create_info, NULL, &context->pipeline_layout);
	assert(result == VK_SUCCESS);

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = 2;
	graphics_pipeline_create_info.pStages = shader_stage_create_infos;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterizer_create_info;
	graphics_pipeline_create_info.pMultisampleState = &multisample_create_info;
	graphics_pipeline_create_info.pDepthStencilState = NULL;
	graphics_pipeline_create_info.pColorBlendState = &color_blend_create_info;
	graphics_pipeline_create_info.pDynamicState = &dynamic_state_create_info;
	graphics_pipeline_create_info.layout = context->pipeline_layout;
	graphics_pipeline_create_info.renderPass = context->render_pass;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(context->logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &context->graphics_pipeline);
	assert(result == VK_SUCCESS);

	vkDestroyShaderModule(context->logical_device, fragment_shader_module, NULL);
	vkDestroyShaderModule(context->logical_device, vertex_shader_module, NULL);	
}

void create_frame_buffers(VkContext *context){
	context->frame_buffers = malloc(sizeof(VkFramebuffer) * context->swapchain_info.image_count);
	for(size_t i = 0; i < context->swapchain_info.image_count; i++){
		VkImageView image_view = context->image_views[i];
		VkFramebufferCreateInfo frame_buffer_create_info = {};
		frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_info.renderPass = context->render_pass;
		frame_buffer_create_info.attachmentCount = 1;
		frame_buffer_create_info.pAttachments = &image_view;
		frame_buffer_create_info.width = context->swapchain_info.image_extent.width;
		frame_buffer_create_info.height = context->swapchain_info.image_extent.height;
		frame_buffer_create_info.layers = 1;
		VkResult result = vkCreateFramebuffer(context->logical_device, &frame_buffer_create_info, NULL, &context->frame_buffers[i]);
		assert(result == VK_SUCCESS);
	}
}

void create_command_pool(VkContext *context){
	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.flags =  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = context->graphics_queue_index;
	VkResult result = vkCreateCommandPool(context->logical_device, &command_pool_create_info, NULL, &context->command_pool);
	assert(result == VK_SUCCESS);
}

void create_command_buffer(VkContext *context){
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = context->command_pool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;
	VkResult result = vkAllocateCommandBuffers(context->logical_device, &info, &context->command_buffer);
	assert(result == VK_SUCCESS);
}

void record_command_buffer(VkContext *context, uint32_t image_index){
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;
	VkResult result = vkBeginCommandBuffer(context->command_buffer, &begin_info);
	assert(result == VK_SUCCESS);

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = context->render_pass;
	render_pass_info.framebuffer = context->frame_buffers[image_index];
	render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
	render_pass_info.renderArea.extent = context->swapchain_info.image_extent;
	
	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;
	
	vkCmdBeginRenderPass(context->command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(context->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->graphics_pipeline);
	
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = 800;
	viewport.height = 600;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(context->command_buffer, 0, 1, &viewport);
	
	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = context->swapchain_info.image_extent;
	vkCmdSetScissor(context->command_buffer, 0, 1, &scissor);

	vkCmdDraw(context->command_buffer, 3, 1, 0, 0);
	
	vkCmdEndRenderPass(context->command_buffer);
	result = vkEndCommandBuffer(context->command_buffer);
	assert(result == VK_SUCCESS);	
}

void create_sync_objects(VkContext *context){
	VkSemaphoreCreateInfo semaphore_info  = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult result = vkCreateSemaphore(context->logical_device, &semaphore_info, NULL, &context->image_available_semaphore);
	assert(result == VK_SUCCESS);
	result = vkCreateSemaphore(context->logical_device, &semaphore_info, NULL, &context->render_finished_semaphore);
	assert(result == VK_SUCCESS);
	result = vkCreateFence(context->logical_device, &fence_info, NULL, &context->in_flight_fence);
	assert(result == VK_SUCCESS);
}

void draw_frame(VkContext *context){
	vkWaitForFences(context->logical_device, 1, &context->in_flight_fence, VK_TRUE, UINT64_MAX);
	vkResetFences(context->logical_device, 1, &context->in_flight_fence);
	uint32_t image_index;
	vkAcquireNextImageKHR(context->logical_device, context->swapchain, UINT64_MAX, context->image_available_semaphore, VK_NULL_HANDLE, &image_index);	
	vkResetCommandBuffer(context->command_buffer, 0);
	record_command_buffer(context, image_index);
	
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore wait_semaphores[] = {context->image_available_semaphore};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &wait_semaphores[0];
	submit_info.pWaitDstStageMask = &wait_stages[0];
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &context->command_buffer;
	
	VkSemaphore signal_semaphores[] = {context->render_finished_semaphore};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &signal_semaphores[0];
	VkResult result = vkQueueSubmit(context->graphics_queue, 1, &submit_info, context->in_flight_fence);
	assert(result == VK_SUCCESS);	
	
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &signal_semaphores[0];
	
	VkSwapchainKHR swapchains[] = {context->swapchain};
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchains[0];	
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;
	vkQueuePresentKHR(context->present_queue, &present_info);
}

void cleanup_vk_context(VkContext *context){
	if(context->in_flight_fence != NULL){
		vkDestroyFence(context->logical_device, context->in_flight_fence, NULL);
	}
	if(context->render_finished_semaphore != NULL){
		vkDestroySemaphore(context->logical_device, context->render_finished_semaphore, NULL);
	}
	if(context->image_available_semaphore != NULL){
		vkDestroySemaphore(context->logical_device, context->image_available_semaphore, NULL);
	}
	if(context->command_pool != NULL){
		vkDestroyCommandPool(context->logical_device, context->command_pool, NULL);
	}
	if(context->frame_buffers != NULL){
		for(size_t i = 0; i < context->swapchain_info.image_count; i++){
			vkDestroyFramebuffer(context->logical_device, context->frame_buffers[i], NULL);
		}
		free(context->frame_buffers);
	}
	if(context->graphics_pipeline != NULL){
		vkDestroyPipeline(context->logical_device, context->graphics_pipeline, NULL);
	}
	if(context->pipeline_layout != NULL){
		vkDestroyPipelineLayout(context->logical_device, context->pipeline_layout, NULL);
	}
	if(context->render_pass != NULL){
		vkDestroyRenderPass(context->logical_device, context->render_pass, NULL);
	}
	if(context->image_views != NULL){
		for(uint32_t i = 0; i < context->swapchain_info.image_count; i++){
			vkDestroyImageView(context->logical_device, context->image_views[i], NULL);
		}
		free(context->image_views);
	}	
	if(context->images != NULL){
		free(context->images);
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


void initialise_vulkan_context(VkContext *context){
	create_window(context);
	create_vk_instance(context);
	create_surface(context);
	pick_physical_device(context);
	create_logical_device(context);
	create_swapchain(context);
	create_images(context);
	create_image_views(context);
	create_render_pass(context);
	create_graphics_pipeline(context);
	create_frame_buffers(context);
	create_command_pool(context);
	create_command_buffer(context);
	create_sync_objects(context);
}

int main(){
	glfwInit();

	VkContext context = {};
	initialise_vulkan_context(&context);
	while(!glfwWindowShouldClose(context.window)) {
        glfwPollEvents();
		draw_frame(&context);
    }
	vkDeviceWaitIdle(context.logical_device);
	cleanup_vk_context(&context);
	
	glfwTerminate();
    return 0;
}


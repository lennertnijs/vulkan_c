#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "aurora_internal.h"
#include "file.c"

const int validation_layer_count = 1;
const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const int extension_count = 1;
const char* extensions[] = {"VK_KHR_swapchain"};
const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t current_frame = 0;

void create_window(VkSession* session) {
	assert(session != NULL);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // dont use openGL
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    session->window = glfwCreateWindow(800, 600, "Vulkan", NULL, NULL);
	assert(session->window != NULL);
}

void create_surface(VkSession *session){
	assert(session != NULL);
	VkResult result = glfwCreateWindowSurface(session->instance, session->window, NULL, &session->surface);
	assert(result == VK_SUCCESS);
}

bool supports_validation_layers(){
    uint32_t supported_layer_count = 0;
    vkEnumerateInstanceLayerProperties(&supported_layer_count, NULL);
    VkLayerProperties *supported_layers = malloc(sizeof(VkLayerProperties) * supported_layer_count);
    vkEnumerateInstanceLayerProperties(&supported_layer_count, supported_layers);
    
    for(int i = 0; i < validation_layer_count; i++){
		bool layer_supported = false;
		for(uint32_t j = 0; j < supported_layer_count; j++){
			if(strcmp(validation_layers[i], supported_layers[j].layerName) == 0){
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

void create_vk_instance(VkConfig *config, VkSession *session){
	if(config->enable_validation_layers && !supports_validation_layers()){
		printf("Validation layers are not supported.\n");
		abort();
	}
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = config->application_name;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Not an engine";
	
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = config->glfw_extension_count;
    create_info.ppEnabledExtensionNames = config->glfw_extensions;
	if(config->enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}

	if(vkCreateInstance(&create_info, NULL, &session->instance) != VK_SUCCESS){
		printf("VkInstance creation failed.\n");
		abort();
	}
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


void select_physical_device(VkSession *session){
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(session->instance, &count, NULL);
	VkPhysicalDevice *physical_devices = malloc(sizeof(VkPhysicalDevice) * count);
	vkEnumeratePhysicalDevices(session->instance, &count, physical_devices);
	
	for(uint32_t i = 0; i < count; i++){
		if(!has_graphics_queue(physical_devices[i])){
			continue;
		}
		if(!has_present_queue(physical_devices[i], session->surface)){
			continue;
		}
		if(!supports_extensions(physical_devices[i], extensions, extension_count)){
			continue;
		}
		if(!supports_surface_format(physical_devices[i], session->surface)){
			continue;
		}
		session->physical_device = physical_devices[i];
		free(physical_devices);
		return;
	}
	free(physical_devices);
	printf("No suitable physical device was found.\n");
}

void create_logical_device(VkConfig *config, VkSession *session){
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(session->physical_device, &count, NULL);
	if(count == 0){
		printf("The physical device supports no queue families.\n");
		abort();
	}
	VkQueueFamilyProperties *properties = malloc(sizeof(VkQueueFamilyProperties) * count);
	vkGetPhysicalDeviceQueueFamilyProperties(session->physical_device, &count, properties);
	
	session->graphics_queue_index = UINT32_MAX;
	session->present_queue_index = UINT32_MAX;


	VkBool32 supports_presenting = false;
	for(uint32_t i = 0; i < count; i++){
		if((properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0){
			session->graphics_queue_index = i;			
		}
		vkGetPhysicalDeviceSurfaceSupportKHR(session->physical_device, i, session->surface, &supports_presenting);
		if(supports_presenting){	
			session->present_queue_index = i;	
		}
	}
	free(properties);
	if(session->graphics_queue_index == UINT32_MAX){
		printf("No graphics queue was found.\n");
		abort();
	}
	if(session->present_queue_index == UINT32_MAX){
		printf("No present queue was found. Try allowing queue sharing.\n");
		abort();
	}

	float priority = 1.0f;		
	bool queue_shared = session->graphics_queue_index == session->present_queue_index;
	int queue_count = queue_shared ? 1 : 2;
	VkDeviceQueueCreateInfo *queue_infos = malloc(sizeof(VkDeviceQueueCreateInfo) * queue_count);
	queue_infos[0] = (VkDeviceQueueCreateInfo){
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = session->graphics_queue_index,
		.queueCount = 1,
		.pQueuePriorities = &priority
	};
	if(!queue_shared){	
		queue_infos[1] = (VkDeviceQueueCreateInfo){
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = session->present_queue_index,
			.queueCount = 1,
			.pQueuePriorities = &priority
		};
	}
	
	VkPhysicalDeviceFeatures device_features = {};

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pQueueCreateInfos = queue_infos;
	create_info.queueCreateInfoCount = queue_count;
	create_info.pEnabledFeatures = &device_features;
	create_info.enabledExtensionCount = extension_count;
	create_info.ppEnabledExtensionNames = extensions;
	if(config->enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	}else{
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}
	if(vkCreateDevice(session->physical_device, &create_info, NULL, &session->logical_device) != VK_SUCCESS){
		printf("Logical device creation failed.\n");
		abort();
	}
	vkGetDeviceQueue(session->logical_device, session->graphics_queue_index, 0, &session->graphics_queue);
	vkGetDeviceQueue(session->logical_device, session->present_queue_index, 0, &session->present_queue);
}

void create_swapchain(VkSession *session){
	VkSurfaceCapabilitiesKHR capabilities= {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(session->physical_device, session->surface, &capabilities);

	session->image_count = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount != 0 && session->image_count > capabilities.maxImageCount){
		session->image_count = capabilities.minImageCount;
	
	if(capabilities.currentExtent.width != UINT32_MAX){
		session->image_extent = capabilities.currentExtent;
	}else{
		// already set beforehand to keep glfw the fuck out of here
	}

	uint32_t format_count = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(session->physical_device, session->surface, &format_count, NULL);
	if(format_count == 0){
		printf("No swapchain image format supported.\n");
		abort();
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
		printf("No swapchain present mode supported.\n");
		exit(1);
	}
	VkPresentModeKHR *present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(session->physical_device, session->surface, &present_mode_count, present_modes);
	
	VkPresentModeKHR present_mode = present_modes[0];
	for(uint32_t i = 0; i < present_mode_count; i++){
		if(present_modes[i] == VK_PRESENT_MODE_FIFO_KHR){
			present_mode = VK_PRESENT_MODE_FIFO_KHR;	
			break;
		}
	}
	
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
	indices[0] = session->graphics_queue_index;
	indices[1] = session->present_queue_index; 
	if(indices[0] != indices[1]){
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
	if(vkCreateSwapchainKHR(session->logical_device, &create_info, NULL, &session->swapchain) != VK_SUCCESS){
		printf("Swapchain creation failed.\n");
		abort();
	}

	uint32_t count = 0;
	vkGetSwapchainImagesKHR(session->logical_device, session->swapchain, &count, NULL);
	if(count == 0){
		printf("No images in the swapchain.\n");
		abort();
	}
	session->images = malloc(sizeof(VkImage) * count);
	vkGetSwapchainImagesKHR(session->logical_device, session->swapchain, &count, session->images);
	}
}

void create_image_views(VkSession *session){
	session->image_views = malloc(sizeof(VkImageView) * session->image_count);
	for(int i = 0; i < session->image_count; i++){
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
		if(vkCreateImageView(session->logical_device, &create_info, NULL, &session->image_views[i])){
			printf("Image view creation failed.\n");
			abort();
		}
	}
}


void create_render_pass(VkSession *session){
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = session->image_format.format;
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
	if(vkCreateRenderPass(session->logical_device, &render_pass_create_info, NULL, &session->render_pass)){
		printf("Failed to create a render pass.\n");
		abort();
	}
}

VkShaderModule create_shader_module(VkSession *session, char* code, size_t length){
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = length;
	create_info.pCode = (uint32_t*)code; // align the char* to uint32_t (so, 4x the space from 1 byte to 4 bytes)
	VkShaderModule shader_module;
	if(vkCreateShaderModule(session->logical_device, &create_info, NULL, &shader_module) != VK_SUCCESS){
		return NULL;
	}
	return shader_module;
}

VkVertexInputBindingDescription get_binding_description(){
	VkVertexInputBindingDescription description = {};
	description.binding = 0;
	description.stride = sizeof(Vertex);
	description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return description;
}

VkVertexInputAttributeDescription* get_attribute_descriptions(){
	VkVertexInputAttributeDescription description1 = {
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(Vertex, position)
	};

	VkVertexInputAttributeDescription description2 = {
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, color)
	};
	
	VkVertexInputAttributeDescription* attribute_descriptions = malloc(sizeof(VkVertexInputAttributeDescription) * 2);
	attribute_descriptions[0] = description1;
	attribute_descriptions[1] = description2;
	return attribute_descriptions;
}

void create_graphics_pipeline(VkConfig *config, VkSession *session){
	size_t vert_shader_length = fetch_file_size("src/vert.spv");
	char *vert_shader_code = read_file("src/vert.spv", vert_shader_length);
	size_t frag_shader_length = fetch_file_size("src/frag.spv");
	char *frag_shader_code = read_file("src/frag.spv", frag_shader_length);
	VkShaderModule vertex_shader_module = create_shader_module(session, vert_shader_code, vert_shader_length);
	VkShaderModule fragment_shader_module = create_shader_module(session, frag_shader_code, frag_shader_length);
	
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
	
	VkVertexInputBindingDescription binding_description = get_binding_description();
	VkVertexInputAttributeDescription* attribute_descriptions = get_attribute_descriptions();
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = 2;
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
	
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float) session->image_extent.width;
	viewport.height = (float) session->image_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	VkOffset2D offset = {0, 0};
	scissor.offset = offset;
	scissor.extent = session->image_extent;
	
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
	rasterizer_create_info.cullMode = VK_CULL_MODE_NONE;
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
	VkResult result = vkCreatePipelineLayout(session->logical_device, &pipeline_layout_create_info, NULL, &session->pipeline_layout);
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
	graphics_pipeline_create_info.layout = session->pipeline_layout;
	graphics_pipeline_create_info.renderPass = session->render_pass;
	graphics_pipeline_create_info.subpass = 0;
	graphics_pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	graphics_pipeline_create_info.basePipelineIndex = -1;

	result = vkCreateGraphicsPipelines(session->logical_device, VK_NULL_HANDLE, 1, &graphics_pipeline_create_info, NULL, &session->graphics_pipeline);
	assert(result == VK_SUCCESS);
	free(attribute_descriptions);
	vkDestroyShaderModule(session->logical_device, fragment_shader_module, NULL);
	vkDestroyShaderModule(session->logical_device, vertex_shader_module, NULL);	
}


void create_framebuffers(VkSession *session){
	session->frame_buffers = malloc(sizeof(VkFramebuffer) * session->image_count);
	for(size_t i = 0; i < session->image_count; i++){
		VkImageView image_view = session->image_views[i];
		VkFramebufferCreateInfo frame_buffer_create_info = {};
		frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_info.renderPass = session->render_pass;
		frame_buffer_create_info.attachmentCount = 1;
		frame_buffer_create_info.pAttachments = &image_view;
		frame_buffer_create_info.width = session->image_extent.width;
		frame_buffer_create_info.height = session->image_extent.height;
		frame_buffer_create_info.layers = 1;
		VkResult result = vkCreateFramebuffer(session->logical_device, &frame_buffer_create_info, NULL, &session->frame_buffers[i]);
		assert(result == VK_SUCCESS);
	}
}


void create_command_pool(VkSession *session){
	VkCommandPoolCreateInfo command_pool_create_info = {};
	command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	command_pool_create_info.flags =  VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	command_pool_create_info.queueFamilyIndex = session->graphics_queue_index;
	VkResult result = vkCreateCommandPool(session->logical_device, &command_pool_create_info, NULL, &session->command_pool);
	assert(result == VK_SUCCESS);
}

void allocate_command_buffers(VkSession *session){
	session->command_buffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = session->command_pool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
	VkResult result = vkAllocateCommandBuffers(session->logical_device, &info, &session->command_buffers[0]);
	assert(result == VK_SUCCESS);
}


void record_command_buffer(VkConfig *config, VkSession *session, uint32_t image_index){
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;
	VkResult result = vkBeginCommandBuffer(session->command_buffers[current_frame], &begin_info);
	assert(result == VK_SUCCESS);
	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = session->render_pass;
	render_pass_info.framebuffer = session->frame_buffers[image_index];
	render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
	render_pass_info.renderArea.extent = session->image_extent;
	VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	render_pass_info.clearValueCount = 1;
	render_pass_info.pClearValues = &clear_color;
	vkCmdBeginRenderPass(session->command_buffers[current_frame], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(session->command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, session->graphics_pipeline);
	
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = session->image_extent.width; // todo
	viewport.height = session->image_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(session->command_buffers[current_frame], 0, 1, &viewport);
	VkRect2D scissor = {};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = session->image_extent;
	vkCmdSetScissor(session->command_buffers[current_frame], 0, 1, &scissor);
	
	VkBuffer vertex_buffers[] = {session->vertex_buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(session->command_buffers[current_frame], 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(session->command_buffers[current_frame], session->index_buffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdDrawIndexed(session->command_buffers[current_frame], session->index_count, 1, 0, 0, 0);
	
	vkCmdEndRenderPass(session->command_buffers[current_frame]);
	result = vkEndCommandBuffer(session->command_buffers[current_frame]);
	assert(result == VK_SUCCESS);	
}


uint32_t find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties){
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
	for(uint32_t i = 0; i < memory_properties.memoryTypeCount; i++){
		if((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties){
			return i;
		}
	}
	assert(0 == 1);
}

void create_buffer(VkSession *session, VkDeviceSize size, VkBufferUsageFlags usage, 
				   VkMemoryPropertyFlags properties, VkBuffer* buffer, VkDeviceMemory* buffer_memory){
	VkBufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult result = vkCreateBuffer(session->logical_device, &info, NULL, buffer);
	assert(result == VK_SUCCESS);
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(session->logical_device, *buffer, &requirements);
	
	VkMemoryAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = requirements.size;	
	alloc_info.memoryTypeIndex = find_memory_type(session->physical_device, requirements.memoryTypeBits, properties);
	result = vkAllocateMemory(session->logical_device, &alloc_info, NULL, buffer_memory);
	assert(result == VK_SUCCESS);
	vkBindBufferMemory(session->logical_device, *buffer, *buffer_memory, 0);
}

void copy_buffer(VkSession *session, VkBuffer src, VkBuffer dst, VkDeviceSize size){
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandPool = session->command_pool;
	info.commandBufferCount = 1;
	
	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(session->logical_device, &info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(command_buffer, &begin_info);
	
	VkBufferCopy copy = {};
	copy.srcOffset = 0;
	copy.dstOffset = 0;
	copy.size = size;
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &copy);
	vkEndCommandBuffer(command_buffer);
	
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffer;
	vkQueueSubmit(session->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(session->graphics_queue);
	vkFreeCommandBuffers(session->logical_device, session->command_pool, 1, &command_buffer);
}


void create_vertex_buffer(VkConfig *config, VkSession *session){
	VkDeviceSize buffer_size = sizeof(Vertex) * config->vertex_count;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(session, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
	void* data;
	
	vkMapMemory(session->logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, config->vertices, (size_t) buffer_size);
	vkUnmapMemory(session->logical_device, staging_buffer_memory);

	
	create_buffer(session, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &session->vertex_buffer, &session->vertex_buffer_memory);
	copy_buffer(session, staging_buffer, session->vertex_buffer, buffer_size);

	vkDestroyBuffer(session->logical_device, staging_buffer, NULL);
	vkFreeMemory(session->logical_device, staging_buffer_memory, NULL);
}


void create_index_buffer(VkConfig *config, VkSession *session){
	VkDeviceSize buffer_size = sizeof(uint16_t) * config->index_count;

	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	create_buffer(session, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

	void* data;
	vkMapMemory(session->logical_device, staging_buffer_memory, 0, buffer_size, 0, &data);
	memcpy(data, config->indices, (size_t) buffer_size);
	vkUnmapMemory(session->logical_device, staging_buffer_memory);

	
	create_buffer(session, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &session->index_buffer, &session->index_buffer_memory);
	copy_buffer(session, staging_buffer, session->index_buffer, buffer_size);

	vkDestroyBuffer(session->logical_device, staging_buffer, NULL);
	vkFreeMemory(session->logical_device, staging_buffer_memory, NULL);
}

void create_sync_objects(VkSession *session){
	session->image_available_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	session->render_finished_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	session->in_flight_fences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphore_info  = {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fence_info = {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
		VkResult result = vkCreateSemaphore(session->logical_device, &semaphore_info, NULL, &session->image_available_semaphores[i]);
		assert(result == VK_SUCCESS);
		result = vkCreateSemaphore(session->logical_device, &semaphore_info, NULL, &session->render_finished_semaphores[i]);
		assert(result == VK_SUCCESS);
		result = vkCreateFence(session->logical_device, &fence_info, NULL, &session->in_flight_fences[i]);
		assert(result == VK_SUCCESS);
	}
}


void recreate_swapchain(VkConfig *config, VkSession *session){
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(session->window, &width, &height);
	while(width == 0 && height == 0){
		glfwGetFramebufferSize(session->window, &width, &height);
		glfwWaitEvents();
	}
	vkDeviceWaitIdle(session->logical_device);
	
	if(session->frame_buffers != NULL){
		for(size_t i = 0; i < session->image_count; i++){
			vkDestroyFramebuffer(session->logical_device, session->frame_buffers[i], NULL);
		}
		free(session->frame_buffers);
	}	
	if(session->image_views != NULL){
		for(uint32_t i = 0; i < session->image_count; i++){
			vkDestroyImageView(session->logical_device, session->image_views[i], NULL);
		}
		free(session->image_views);
	}	
	if(session->swapchain != NULL){
		vkDestroySwapchainKHR(session->logical_device, session->swapchain, NULL);
	}

	create_swapchain(session);
	create_image_views(session);
	create_framebuffers(session);	
}


void vulkan_session_draw_frame(VkConfig *config, VkSession *session, bool resized){
	vkWaitForFences(session->logical_device, 1, &session->in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
	uint32_t image_index;
	VkResult res = vkAcquireNextImageKHR(session->logical_device, session->swapchain, UINT64_MAX, session->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);	
	if(res == VK_ERROR_OUT_OF_DATE_KHR){
		recreate_swapchain(config, session);
		return;
	}
	assert(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);
	vkResetFences(session->logical_device, 1, &session->in_flight_fences[current_frame]);
	vkResetCommandBuffer(session->command_buffers[current_frame], 0);
	record_command_buffer(config, session, image_index);
	
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore wait_semaphores[] = {session->image_available_semaphores[current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &wait_semaphores[0];
	submit_info.pWaitDstStageMask = &wait_stages[0];
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &session->command_buffers[current_frame];
	
	VkSemaphore signal_semaphores[] = {session->render_finished_semaphores[current_frame]};
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &signal_semaphores[0];
	VkResult result = vkQueueSubmit(session->graphics_queue, 1, &submit_info,session->in_flight_fences[current_frame]);
	assert(result == VK_SUCCESS);	
	
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &signal_semaphores[0];

	VkSwapchainKHR swapchains[] = {session->swapchain};
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchains[0];	
	present_info.pImageIndices = &image_index;
	present_info.pResults = NULL;
	res = vkQueuePresentKHR(session->present_queue, &present_info);
	if(res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR || resized){
		resized = false;
		recreate_swapchain(config, session);	
	}else if(res != VK_SUCCESS){
		printf("Error\n");
		exit(1);
	}
	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void init_vertices(VkConfig *config, VkSession *session){
	session->vertices = config->vertices;
	session->vertex_count = config->vertex_count;
	session->indices = config->indices;
	session->index_count = config->index_count;
}


void add_vertices(VkSession *session, Vertex *vertices, int vertex_count, uint16_t *indices, int index_count){
	vkDeviceWaitIdle(session->logical_device);
	session->vertices = realloc(session->vertices, sizeof(Vertex) * (session->vertex_count + vertex_count));	
	for(int i = 0; i < vertex_count; i++){
		session->vertices[session->vertex_count + i] = vertices[i];
	}
	session->vertex_count += vertex_count;
	session->indices = realloc(session->indices, sizeof(uint16_t) * (session->index_count + index_count));
	for(int i = 0; i < index_count; i++){
		session->indices[session->index_count + i] = indices[i];
	}
	session->index_count += index_count;
	

	vkDestroyBuffer(session->logical_device, session->vertex_buffer, NULL);
	vkFreeMemory(session->logical_device, session->vertex_buffer_memory, NULL);
	vkDestroyBuffer(session->logical_device, session->index_buffer, NULL);
	vkFreeMemory(session->logical_device, session->index_buffer_memory, NULL);

	create_vertex_buffer(NULL, session);
	create_index_buffer(NULL, session); // todo
}

VkSession *vulkan_session_create(VkConfig *config){
	if(config == NULL){
		printf("Vulkan config is NULL.");
	}
	VkSession *session = malloc(sizeof(VkSession));
	init_vertices(config, session);
	create_vk_instance(config, session);
	create_window(session);
	create_surface(session); // fix this
	select_physical_device(session);
	create_logical_device(config, session);
	create_swapchain(session);
	create_image_views(session);
	create_render_pass(session);
	create_graphics_pipeline(config, session);
	create_framebuffers(session);
	create_command_pool(session);
	create_vertex_buffer(config, session);
	create_index_buffer(config, session);
	allocate_command_buffers(session);
	create_sync_objects(session);
	return session;
}

GLFWwindow *vulkan_session_get_window(VkSession *session){	
	return session->window;
}

void vulkan_session_destroy(VkSession *session){
	vkDeviceWaitIdle(session->logical_device);
		for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
			vkDestroyFence(session->logical_device, session->in_flight_fences[i], NULL);
		}
		for(int i = 0; i <MAX_FRAMES_IN_FLIGHT; i++){
			vkDestroySemaphore(session->logical_device, session->render_finished_semaphores[i], NULL);
		}
		for(int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
			vkDestroySemaphore(session->logical_device, session->image_available_semaphores[i], NULL);
		}

	vkDestroyCommandPool(session->logical_device, session->command_pool, NULL);

	for(uint32_t i = 0; i < session->image_count; i++){
		vkDestroyFramebuffer(session->logical_device, session->frame_buffers[i], NULL);
	}
	free(session->frame_buffers);
	vkDestroyPipeline(session->logical_device, session->graphics_pipeline, NULL);
	vkDestroyPipelineLayout(session->logical_device, session->pipeline_layout, NULL);
	vkDestroyRenderPass(session->logical_device, session->render_pass, NULL);
	for(uint32_t i = 0; i < session->image_count; i++){
		vkDestroyImageView(session->logical_device, session->image_views[i], NULL);
	}
	free(session->image_views);
	free(session->images);
	vkDestroySwapchainKHR(session->logical_device, session->swapchain, NULL);
	vkDestroyBuffer(session->logical_device, session->vertex_buffer, NULL);
	vkFreeMemory(session->logical_device, session->vertex_buffer_memory, NULL);
	vkDestroyBuffer(session->logical_device, session->index_buffer, NULL);
	vkFreeMemory(session->logical_device, session->index_buffer_memory, NULL);
	vkDestroyDevice(session->logical_device, NULL);
	vkDestroySurfaceKHR(session->instance, session->surface, NULL);
	vkDestroyInstance(session->instance, NULL);
	glfwDestroyWindow(session->window);
	glfwTerminate();
	free(session);
}

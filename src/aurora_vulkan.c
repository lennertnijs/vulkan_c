#include <vulkan/vulkan.h>
#include <glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <cglm/cglm.h>
#include "stb_image.h"
#include "tinyobj_loader_c.h"
#include "aurora_internal.h"
#include "io.h"
#include "buffer.h"

const int validation_layer_count = 1;
const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
const int extension_count = 1;
const char* extensions[] = {"VK_KHR_swapchain"};
const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t current_frame = 0;

const char* MODEL_PATH = "D:/vulkan_c/src/models/viking_room.obj";
const char* TEXTURE_PATH = "D:/vulkan_c/src/textures/viking_room.png";

typedef struct {
	mat4 model;
	mat4 view;
	mat4 proj;
} UniformBufferObject;

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

VkCommandBuffer begin_single_time_commands(VkSession* session) {
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = 0,
		.commandPool = session->command_pool,
		.commandBufferCount = 1,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(session->logical_device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(command_buffer, &begin_info);
	return command_buffer;
}

void end_single_time_commands(VkSession* session, VkCommandBuffer command_buffer) {
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer
	};
	vkQueueSubmit(session->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(session->graphics_queue);
	vkFreeCommandBuffers(session->logical_device, session->command_pool, 1, &command_buffer);
}

void create_vk_instance(VkConfig *config, VkSession *session){
	if(config->enable_validation_layers && !supports_validation_layers()){
		printf("Validation layers are not supported.\n");
		abort();
	}
    VkApplicationInfo app_info = {0};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = config->application_name;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Not an engine";
	app_info.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	
    VkInstanceCreateInfo create_info = {0};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = config->glfw_extension_count;
    create_info.ppEnabledExtensionNames = config->glfw_extensions;
	//if(config->enable_validation_layers){
		create_info.enabledLayerCount = validation_layer_count;
		create_info.ppEnabledLayerNames = validation_layers;
	//}else{
	//	create_info.enabledLayerCount = 0;
	//	create_info.ppEnabledLayerNames = NULL;
	//}

	if(vkCreateInstance(&create_info, NULL, &session->instance) != VK_SUCCESS){
		printf("VkInstance creation failed.\n");
		abort();
	}
}

bool has_graphics_queue(VkPhysicalDevice physical_device){
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &count, NULL);
	VkQueueFamilyProperties *properties = malloc(sizeof(VkQueueFamilyProperties) * count);
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

	VkBool32 supports_presenting = VK_FALSE;
	for(uint32_t i = 0; i < count; i++){	
		vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &supports_presenting);
		if(supports_presenting == VK_TRUE){
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

	VkPhysicalDeviceFeatures physical_device_features;
	vkGetPhysicalDeviceFeatures(session->physical_device, &physical_device_features);
	if (!physical_device_features.samplerAnisotropy) {
		printf("No anisotropy sampler supported!");
		exit(1);
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
	
	VkPhysicalDeviceFeatures device_features = {
		.samplerAnisotropy = VK_TRUE
	};

	VkDeviceCreateInfo create_info = {0};
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

void create_swapchain(VkSession *session)
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(session->physical_device, session->surface, &capabilities);

	session->image_count = capabilities.minImageCount + 1;
	if(capabilities.maxImageCount != 0 && session->image_count > capabilities.maxImageCount){
		session->image_count = capabilities.minImageCount;
	}
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
	
	VkSwapchainCreateInfoKHR create_info = {0}; 
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
	printf("%u\n", count);
	session->images = malloc(sizeof(VkImage) * count);
	vkGetSwapchainImagesKHR(session->logical_device, session->swapchain, &count, session->images);
}

void create_image_views(VkSession *session){
	session->image_views = malloc(sizeof(VkImageView) * session->image_count);
	for(uint32_t i = 0; i < session->image_count; i++){
		VkImageViewCreateInfo create_info = {0};
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
	VkAttachmentDescription color_attachment = {
		.format = session->image_format.format, // swapchain image format
		.samples = VK_SAMPLE_COUNT_1_BIT, // 1 sample (no multisampling yet)
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // clear the framebuffer (to black) before drawing in this render pass
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR // to be presented in the swapchain
	};

	VkAttachmentReference color_attachment_reference = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};
	
	VkAttachmentDescription depth_attachment = {
		.format = find_depth_format(session),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depth_attachment_reference = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_reference, // The fragment shader layout(location = X) directly maps to the index X inside pColorAttachments
		.pDepthStencilAttachment = &depth_attachment_reference,
	};

	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;	
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT; 

	VkAttachmentDescription attachment_descriptions[2] = { color_attachment, depth_attachment };
	VkRenderPassCreateInfo render_pass_create_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = &attachment_descriptions,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};
	if(vkCreateRenderPass(session->logical_device, &render_pass_create_info, NULL, &session->render_pass)){
		printf("Failed to create a render pass.\n");
		abort();
	}
}

VkShaderModule create_shader_module(VkSession *session, char* code, size_t length){
	VkShaderModuleCreateInfo create_info = {0};
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
	VkVertexInputBindingDescription description = {
		.binding = 0,
		.stride = sizeof(Vertex),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
	};
	return description;
}

VkVertexInputAttributeDescription* get_attribute_descriptions(){
	VkVertexInputAttributeDescription description1 = {
		.location = 0,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, position)
	};

	VkVertexInputAttributeDescription description2 = {
		.location = 1,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, color)
	};

	VkVertexInputAttributeDescription description3 = {
		.location = 2,
		.binding = 0,
		.format = VK_FORMAT_R32G32_SFLOAT,
		.offset = offsetof(Vertex, text_coord)
	};
	
	VkVertexInputAttributeDescription* attribute_descriptions = malloc(sizeof(VkVertexInputAttributeDescription) * 3);
	if (attribute_descriptions == 0) {
		printf("Something went wrong mallocing the vertex input attribute descriptions");
		exit(1);
	}
	attribute_descriptions[0] = description1;
	attribute_descriptions[1] = description2;
	attribute_descriptions[2] = description3;
	return attribute_descriptions;
}

void create_graphics_pipeline(VkSession *session){
	size_t vert_shader_length = fetch_file_size("D:/vulkan_c/src/shader/vert.spv");
	char *vert_shader_code = read_file("D:/vulkan_c/src/shader/vert.spv", vert_shader_length);
	size_t frag_shader_length = fetch_file_size("D:/vulkan_c/src/shader/frag.spv");
	char *frag_shader_code = read_file("D:/vulkan_c/src/shader/frag.spv", frag_shader_length);
	VkShaderModule vertex_shader_module = create_shader_module(session, vert_shader_code, vert_shader_length);
	VkShaderModule fragment_shader_module = create_shader_module(session, frag_shader_code, frag_shader_length);
	
	VkPipelineShaderStageCreateInfo vertex_shader_create_info = {0};
	vertex_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertex_shader_create_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertex_shader_create_info.module = vertex_shader_module;
	vertex_shader_create_info.pName = "main";
	
	VkPipelineShaderStageCreateInfo fragment_shader_create_info = {0};
	fragment_shader_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragment_shader_create_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragment_shader_create_info.module = fragment_shader_module;
	fragment_shader_create_info.pName = "main";
	
	VkPipelineShaderStageCreateInfo shader_stage_create_infos[] = {vertex_shader_create_info, fragment_shader_create_info};	
	
	VkVertexInputBindingDescription binding_description = get_binding_description();
	VkVertexInputAttributeDescription* attribute_descriptions = get_attribute_descriptions();
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
	vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = 3;
	vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info = {0};
	input_assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly_create_info.primitiveRestartEnable = VK_FALSE;
	
	VkViewport viewport = {0};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (float) session->image_extent.width;
	viewport.height = (float) session->image_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {0};
	VkOffset2D offset = {0, 0};
	scissor.offset = offset;
	scissor.extent = session->image_extent;
	
	VkPipelineViewportStateCreateInfo viewport_state_create_info = {0};

	viewport_state_create_info.sType = 	VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state_create_info.viewportCount = 1;	
	viewport_state_create_info.pViewports = &viewport;
	viewport_state_create_info.scissorCount = 1;
	viewport_state_create_info.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer_create_info = {0};
	rasterizer_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer_create_info.depthClampEnable = VK_FALSE;
	rasterizer_create_info.rasterizerDiscardEnable = VK_FALSE;
	rasterizer_create_info.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer_create_info.lineWidth = 1.0f;
	rasterizer_create_info.cullMode = VK_CULL_MODE_NONE;
	rasterizer_create_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer_create_info.depthBiasEnable = VK_FALSE;
	rasterizer_create_info.depthBiasConstantFactor = 0.0f;
	rasterizer_create_info.depthBiasClamp = 0.0f;
	rasterizer_create_info.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisample_create_info = {0};
	multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample_create_info.sampleShadingEnable = VK_FALSE;
	multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisample_create_info.minSampleShading = 1.0f;
	multisample_create_info.pSampleMask = NULL;
	multisample_create_info.alphaToCoverageEnable = VK_FALSE;
	multisample_create_info.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState color_blend_attachment_create_info = {0};
	color_blend_attachment_create_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment_create_info.blendEnable = VK_FALSE;
	color_blend_attachment_create_info.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_create_info.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_create_info.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment_create_info.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment_create_info.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment_create_info.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blend_create_info = {0};
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
	VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {0};
	dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state_create_info.dynamicStateCount = 2;
	dynamic_state_create_info.pDynamicStates = &states[0];


	VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
	pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_create_info.setLayoutCount = 1; 
	pipeline_layout_create_info.pSetLayouts = &session->descriptor_set_layout;
	pipeline_layout_create_info.pushConstantRangeCount = 0;
	pipeline_layout_create_info.pPushConstantRanges = NULL;
	VkResult result = vkCreatePipelineLayout(session->logical_device, &pipeline_layout_create_info, NULL, &session->pipeline_layout);
	assert(result == VK_SUCCESS);

	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE,
		.front = {0},
		.back = {0}
	};

	VkGraphicsPipelineCreateInfo graphics_pipeline_create_info = {0};
	graphics_pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphics_pipeline_create_info.stageCount = 2;
	graphics_pipeline_create_info.pStages = shader_stage_create_infos;
	graphics_pipeline_create_info.pVertexInputState = &vertex_input_info;
	graphics_pipeline_create_info.pInputAssemblyState = &input_assembly_create_info;
	graphics_pipeline_create_info.pViewportState = &viewport_state_create_info;
	graphics_pipeline_create_info.pRasterizationState = &rasterizer_create_info;
	graphics_pipeline_create_info.pMultisampleState = &multisample_create_info;
	graphics_pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
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
	if (session->frame_buffers == 0) {
		exit(1);
	}
	for(size_t i = 0; i < session->image_count; i++){
		VkImageView views[2] = { session->image_views[i], session->depth_image_view };
		VkFramebufferCreateInfo frame_buffer_create_info = {0};
		frame_buffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frame_buffer_create_info.renderPass = session->render_pass;
		frame_buffer_create_info.attachmentCount = 2;
		frame_buffer_create_info.pAttachments = views;
		frame_buffer_create_info.width = session->image_extent.width;
		frame_buffer_create_info.height = session->image_extent.height;
		frame_buffer_create_info.layers = 1;
		if (vkCreateFramebuffer(session->logical_device, &frame_buffer_create_info, NULL, &session->frame_buffers[i]) != VK_SUCCESS) {
			printf("Failed to create a frame buffer!");
			exit(1);
		}
	}
}


void create_command_pool(VkSession *session){
	VkCommandPoolCreateInfo command_pool_create_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = session->graphics_queue_index
	};
	if (vkCreateCommandPool(session->logical_device, &command_pool_create_info, NULL, &session->command_pool) != VK_SUCCESS) {
		printf("Failed to create a command pool!");
		abort();
	}
}

void allocate_command_buffers(VkSession *session)
{
	session->command_buffers = malloc(sizeof(VkCommandBuffer) * MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = session->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = MAX_FRAMES_IN_FLIGHT
	};
	VkResult result = vkAllocateCommandBuffers(session->logical_device, &info, &session->command_buffers[0]);
	assert(result == VK_SUCCESS);
}


void record_command_buffer(VkSession *session, uint32_t image_index){
	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = NULL;
	VkResult result = vkBeginCommandBuffer(session->command_buffers[current_frame], &begin_info);
	assert(result == VK_SUCCESS);
	VkRenderPassBeginInfo render_pass_info = {0};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.renderPass = session->render_pass;
	render_pass_info.framebuffer = session->frame_buffers[image_index];
	render_pass_info.renderArea.offset = (VkOffset2D){0, 0};
	render_pass_info.renderArea.extent = session->image_extent;
	VkClearValue clear_color = { 0.0f, 0.0f, 0.0f, 1.0f };
	VkClearValue clear_depth_stencil = { 1.0f, 0 };
	VkClearValue clear_values[2] = { clear_color, clear_depth_stencil };
	render_pass_info.clearValueCount = 2;
	render_pass_info.pClearValues = clear_values;
	vkCmdBeginRenderPass(session->command_buffers[current_frame], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(session->command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, session->graphics_pipeline);
	
	VkViewport viewport = {0};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = session->image_extent.width; // todo
	viewport.height = session->image_extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(session->command_buffers[current_frame], 0, 1, &viewport);
	VkRect2D scissor = {0};
	scissor.offset = (VkOffset2D){0, 0};
	scissor.extent = session->image_extent;
	vkCmdSetScissor(session->command_buffers[current_frame], 0, 1, &scissor);
	
	VkBuffer vertex_buffers[] = {session->vertex_buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(session->command_buffers[current_frame], 0, 1, vertex_buffers, offsets);
	vkCmdBindIndexBuffer(session->command_buffers[current_frame], session->index_buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdBindDescriptorSets(session->command_buffers[current_frame], VK_PIPELINE_BIND_POINT_GRAPHICS, session->pipeline_layout, 0, 1, &session->descriptor_sets[current_frame], 0, 0);
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

void create_buffer( VkSession *session, 
					VkDeviceSize size, 
					VkBufferUsageFlags usage, 
					VkMemoryPropertyFlags properties, 
					VkBuffer* buffer, 
					VkDeviceMemory* buffer_memory)
{
	VkBufferCreateInfo info = {0};
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VkResult result = vkCreateBuffer(session->logical_device, &info, NULL, buffer);
	assert(result == VK_SUCCESS);
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(session->logical_device, *buffer, &requirements);
	
	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = requirements.size;	
	alloc_info.memoryTypeIndex = find_memory_type(session->physical_device, requirements.memoryTypeBits, properties);
	result = vkAllocateMemory(session->logical_device, &alloc_info, NULL, buffer_memory);
	assert(result == VK_SUCCESS);
	vkBindBufferMemory(session->logical_device, *buffer, *buffer_memory, 0);
}

void create_sync_objects(VkSession *session){
	session->image_available_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	session->render_finished_semaphores = malloc(sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	session->in_flight_fences = malloc(sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphore_info  = {0};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fence_info = {0};
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

void cleanup_swapchain(VkSession* session) {
	vkDestroyImageView(session->logical_device, session->depth_image_view, 0);
	vkDestroyImage(session->logical_device, session->depth_image, 0);
	vkFreeMemory(session->logical_device, session->depth_image_memory, 0);
}


void recreate_swapchain(VkSession *session){
	int width = 0;
	int height = 0;
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
	cleanup_swapchain(session);
	create_swapchain(session);
	create_image_views(session);
	create_framebuffers(session);	
}

float elapsed = 0.01f;

void update_uniform_buffer(VkSession* session, uint32_t current_image) {
	if (elapsed > 360.0f) elapsed -= 360.0f;
	UniformBufferObject ubo = { 0 };
	glm_mat4_identity(ubo.model);
	glm_rotate(&ubo.model, elapsed * glm_rad(90.0f), (vec3) { 0.0f, 0.0f, 1.0f });

	glm_lookat(
		(vec3) { 2.0f, 2.0f, 2.0f },
		(vec3) { 0.0f, 0.0f, 0.0f },
		(vec3) { 0.0f, 0.0f, 1.0f },
		&ubo.view
	);

	glm_perspective(glm_rad(45.0f),	session->image_extent.width / (float)session->image_extent.height, 0.1f, 10.0f,	&ubo.proj);

	ubo.proj[1][1] *= -1;
	elapsed += 0.01f;
	memcpy(session->uniform_buffers_mapped[current_image], &ubo, sizeof(ubo));
}


void vulkan_session_draw_frame(VkSession *session, bool resized){
	vkWaitForFences(session->logical_device, 1, &session->in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);
	uint32_t image_index;
	VkResult res = vkAcquireNextImageKHR(session->logical_device, session->swapchain, UINT64_MAX, session->image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);	
	if(res == VK_ERROR_OUT_OF_DATE_KHR){
		recreate_swapchain(session);
		return;
	}
	assert(res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR);
	vkResetFences(session->logical_device, 1, &session->in_flight_fences[current_frame]);
	vkResetCommandBuffer(session->command_buffers[current_frame], 0);
	record_command_buffer(session, image_index);
	update_uniform_buffer(session, current_frame);

	VkSubmitInfo submit_info = {0};
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
	
	VkPresentInfoKHR present_info = {0};
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
		recreate_swapchain(session);	
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

void createDescriptorSetLayout(VkSession* session) {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = 0
	};
	VkDescriptorSetLayoutBinding sampler_layout_binding = {
		.binding = 1,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImmutableSamplers = 0,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
	};
	VkDescriptorSetLayoutBinding* bindings = malloc(sizeof(VkDescriptorSetLayoutBinding) * 2);
	if (bindings == 0) {
		printf("Failed to malloc memory for bindings!");
		exit(1);
	}
	bindings[0] = uboLayoutBinding;
	bindings[1] = sampler_layout_binding;
	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 2,
		.pBindings = bindings
	};
	if (vkCreateDescriptorSetLayout(session->logical_device, &layout_info, 0, &session->descriptor_set_layout) != VK_SUCCESS) {
		printf("Failed to create descriptor set!");
		exit(1);
	}
}

void create_uniform_buffers(VkSession* session) {
	VkDeviceSize buffer_size = sizeof(UniformBufferObject);
	session->uniform_buffers = malloc(sizeof(VkBuffer) * MAX_FRAMES_IN_FLIGHT);
	session->uniform_buffers_memory = malloc(sizeof(VkDeviceMemory) * MAX_FRAMES_IN_FLIGHT);
	session->uniform_buffers_mapped = malloc(sizeof(void*) * MAX_FRAMES_IN_FLIGHT);
	session->ubo_count = MAX_FRAMES_IN_FLIGHT;
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		create_buffer(session, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &session->uniform_buffers[i], &session->uniform_buffers_memory[i]);
		vkMapMemory(session->logical_device, session->uniform_buffers_memory[i], 0, buffer_size, 0, &session->uniform_buffers_mapped[i]);
	}
}

void create_descriptor_pool(VkSession* session) {
	VkDescriptorPoolSize pool_size = {
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT
	};
	VkDescriptorPoolSize sampler_pool_size = {
		.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = (uint32_t)MAX_FRAMES_IN_FLIGHT
	};
	VkDescriptorPoolSize* pool_sizes = malloc(sizeof(VkDescriptorPoolSize) * 2);
	if (pool_sizes == 0) {
		printf("Something went wrong mallocating the pool_sizes array for descriptor pools.");
		exit(1);
	}
	pool_sizes[0] = pool_size;
	pool_sizes[1] = sampler_pool_size;

	VkDescriptorPoolCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = 2,
		.pPoolSizes = pool_sizes,
		.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT,
		.flags = 0
	};
	if (vkCreateDescriptorPool(session->logical_device, &create_info, 0, &session->descriptor_pool) != VK_SUCCESS) {
		printf("Descriptor pool set creation failed!\n");
		exit(1);
	}
}

void create_descriptor_sets(VkSession* session) {
	VkDescriptorSetLayout* layouts = malloc(sizeof(VkDescriptorSetLayout) * MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		layouts[i] = session->descriptor_set_layout;
	}
	VkDescriptorSetAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = session->descriptor_pool,
		.descriptorSetCount = (uint32_t)MAX_FRAMES_IN_FLIGHT,
		.pSetLayouts = layouts
	};
	session->descriptor_sets = malloc(sizeof(VkDescriptorSet) * MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(session->logical_device, &alloc_info, session->descriptor_sets) != VK_SUCCESS) {
		printf("Failed to alloc descriptor sets\n");
		exit(1);
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDescriptorBufferInfo buffer_info = {
			.buffer = session->uniform_buffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};
		VkDescriptorImageInfo image_info = {
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.imageView = session->texture_image_view,
			.sampler = session->sampler
		};
		VkWriteDescriptorSet ubo_writer = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = session->descriptor_sets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = &buffer_info,
			.pImageInfo = 0,
			.pTexelBufferView = 0
		};
		VkWriteDescriptorSet image_writer = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = session->descriptor_sets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.pImageInfo = &image_info
		};
		VkWriteDescriptorSet* write_descriptor_sets = malloc(sizeof(VkWriteDescriptorSet) * 2);
		if (write_descriptor_sets == 0) {
			printf("Failed to allocate write descriptor sets");
			exit(1);
		}
		write_descriptor_sets[0] = ubo_writer;
		write_descriptor_sets[1] = image_writer;
		vkUpdateDescriptorSets(session->logical_device, 2, write_descriptor_sets, 0, 0);
	}
}

void create_image(VkSession* session, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage* image, VkDeviceMemory* image_memory) {
	VkImageCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = format,
		.tiling = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.flags = 0
	};
	if (vkCreateImage(session->logical_device, &create_info, 0, image) != VK_SUCCESS) {
		printf("Failed to create vk image for the texture!");
		exit(1);
	}

	VkMemoryRequirements memory_req = { 0 };
	vkGetImageMemoryRequirements(session->logical_device, *image, &memory_req);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memory_req.size,
		.memoryTypeIndex = find_memory_type(session->physical_device, memory_req.memoryTypeBits, properties),
	};
	if (vkAllocateMemory(session->logical_device, &alloc_info, 0, image_memory) != VK_SUCCESS) {
		printf("Failed to allocate texture memory!");
		exit(1);
	}
	vkBindImageMemory(session->logical_device, *image, *image_memory, 0);
}

bool has_stencil_component(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transition_image_layout(VkSession* session, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout) {
	VkCommandBuffer command_buffer = begin_single_time_commands(session);

	VkImageAspectFlags flags;
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		flags = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (has_stencil_component(format)) {
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		flags = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = flags,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = 1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.srcAccessMask = 0, // TODO
		.dstAccessMask = 0
	};

	VkPipelineStageFlags srcStage = { 0 };
	VkPipelineStageFlags dstStage = { 0 };
	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		printf("Invalid pipelines brev!");
		exit(1);
	}
	vkCmdPipelineBarrier(command_buffer, srcStage, dstStage, 0, 0, 0, 0, 0, 1, &barrier);

	end_single_time_commands(session, command_buffer);
}

void copy_buffer_to_image(VkSession* session, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer command_buffer = begin_single_time_commands(session);

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset = { 0, 0, 0 },
		.imageExtent = {
			width,
			height,
			1
		}
	};
	vkCmdCopyBufferToImage(command_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	end_single_time_commands(session, command_buffer);
}

void create_texture_image(VkSession* session) {
	int tex_width, tex_height, tex_channels;
	stbi_uc* pixels = stbi_load(TEXTURE_PATH, &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	VkDeviceSize image_size = tex_width * tex_height * 4;
	if (!pixels) {
		printf("Something went wrong loading the texture.jpg texture!");
		exit(1);
	}
	VkBuffer staging_buffer = { 0 };
	VkDeviceMemory staging_buffer_memory = { 0 };
	create_buffer(session, image_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
	void* data;
	vkMapMemory(session->logical_device, staging_buffer_memory, 0, image_size, 0, &data);
	memcpy(data, pixels, image_size);
	vkUnmapMemory(session->logical_device, staging_buffer_memory);
	stbi_image_free(pixels);

	create_image(session, tex_width, tex_height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &session->texture_image, &session->texture_image_memory);

	transition_image_layout(session, session->texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copy_buffer_to_image(session, staging_buffer, session->texture_image, tex_width, tex_height);
	transition_image_layout(session, session->texture_image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(session->logical_device, staging_buffer, 0);
	vkFreeMemory(session->logical_device, staging_buffer_memory, 0);
}

void create_texture_image_view(VkSession* session) {
	VkImageViewCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = session->texture_image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.subresourceRange = (VkImageSubresourceRange){
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = 1,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};
	if (vkCreateImageView(session->logical_device, &create_info, 0, &session->texture_image_view) != VK_SUCCESS) {
		printf("Something went wrong creating the image view for the textured image");
		exit(1);
	}
}

void create_texture_sampler(VkSession* session) {
	VkPhysicalDeviceProperties physical_device_properties;
	vkGetPhysicalDeviceProperties(session->physical_device, &physical_device_properties);
	VkSamplerCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = physical_device_properties.limits.maxSamplerAnisotropy,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = 0.0f
	};
	if (vkCreateSampler(session->logical_device, &create_info, 0, &session->sampler) != VK_SUCCESS) {
		printf("Failed to create an image sampler.");
		exit(1);
	}
}

VkFormat find_supported_format(VkSession *session, VkFormat* candidates, int amount, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (int i = 0; i < amount; i++) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(session->physical_device, candidates[i], &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return candidates[i];
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return candidates[i];
		}
	}
	printf("No supported formats found!");
	exit(1);
}

VkFormat find_depth_format(VkSession* session) {
	VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return find_supported_format(session, candidates, 3, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void create_depth_resources(VkSession* session) {
	VkFormat format = find_depth_format(session);
	create_image(session, session->image_extent.width, session->image_extent.height, format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &session->depth_image, &session->depth_image_memory);
	VkImageViewCreateInfo create_info = {
	.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	.image = session->depth_image,
	.viewType = VK_IMAGE_VIEW_TYPE_2D,
	.format = format,
	.subresourceRange = (VkImageSubresourceRange){
		.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	}
	};
	if (vkCreateImageView(session->logical_device, &create_info, 0, &session->depth_image_view) != VK_SUCCESS) {
		printf("Something went wrong creating the image view for the textured image");
		exit(1);
	}
	// not required! (taken care in the render pass)
	transition_image_layout(session, session->depth_image, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void tinyobj_file_reader(void* ctx, const char* filename, int is_mtl, const char* obj_filename, char** buf, size_t* len) {
	// Open file in binary mode to avoid newline translation issues
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Error: Could not open file %s\n", filename);
		*buf = NULL;
		*len = 0;
		return;
	}

	// Seek to end to determine size
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size < 0) {
		fclose(file);
		*buf = NULL;
		*len = 0;
		return;
	}

	// Allocate buffer. tinyobjloader-c expects the user to allocate this.
	// Note: If you want to use one of your 2 arenas, swap malloc for your arena_alloc here.
	char* data = (char*)malloc(size + 1);
	if (!data) {
		fclose(file);
		*buf = NULL;
		*len = 0;
		return;
	}

	size_t read_size = fread(data, 1, size, file);
	data[read_size] = '\0'; // Null-terminate just in case, though not strictly required

	*buf = data;
	*len = read_size;

	fclose(file);
}

void load_model(VkSession* session) {
	tinyobj_attrib_t attrib;
	tinyobj_shape_t* shapes = 0;
	size_t num_shapes;
	tinyobj_material_t* materials = 0;
	size_t num_materials;
	tinyobj_attrib_init(&attrib);
	if (tinyobj_parse_obj(&attrib, &shapes, &num_shapes, &materials, &num_materials, MODEL_PATH, tinyobj_file_reader, 0, TINYOBJ_FLAG_TRIANGULATE) != TINYOBJ_SUCCESS) {
		printf("Failed to parse .obj file.");
		exit(1);
	}
	Vertex* vertices = NULL;
	size_t num_vertices = 0;

	unsigned int* indices = NULL;
	size_t num_indices = 0;
	// we do not check for duplicated vertices, so we do not use the index buffer :?
	for (size_t s = 0; s < num_shapes; s++) {
		tinyobj_shape_t* shape = &shapes[s];

		for (unsigned int f = 0; f < shape->length; f++) {
			unsigned int face_idx = shape->face_offset + f;
			unsigned int num_verts_in_face = attrib.face_num_verts[face_idx];

			for (unsigned int v = 0; v < num_verts_in_face; v++) {
				tinyobj_vertex_index_t idx = attrib.faces[face_idx * 3 + v];

				Vertex vertex;

				// Position
				vertex.position.x = attrib.vertices[3 * idx.v_idx + 0];
				vertex.position.y = attrib.vertices[3 * idx.v_idx + 1];
				vertex.position.z = attrib.vertices[3 * idx.v_idx + 2];

				// Texture coordinates (check if valid)
				if (idx.vt_idx >= 0) {
					vertex.text_coord.x = attrib.texcoords[2 * idx.vt_idx + 0];
					vertex.text_coord.y = 1.0f - attrib.texcoords[2 * idx.vt_idx + 1];
				}
				else {
					vertex.text_coord.x = 0.0f;
					vertex.text_coord.y = 0.0f;
				}

				// Color (white)
				vertex.color.x = 1.0f;
				vertex.color.y = 1.0f;
				vertex.color.z = 1.0f;

				// Add vertex
				vertices = (Vertex*)realloc(vertices, (num_vertices + 1) * sizeof(Vertex));
				vertices[num_vertices++] = vertex;

				// Add index
				indices = (unsigned int*)realloc(indices, (num_indices + 1) * sizeof(unsigned int));
				indices[num_indices++] = (unsigned int)(num_indices);
			}
		}
	}
	session->vertices = vertices;
	session->vertex_count = num_vertices;
	session->indices = indices;
	session->index_count = num_indices;
}

VkSession *vulkan_session_create(VkConfig *config){
	if(config == NULL){
		printf("Vulkan config is NULL.");
	}
	VkSession *session = malloc(sizeof(VkSession));
	// init_vertices(config, session);
	create_vk_instance(config, session);
	create_window(session);
	create_surface(session);
	select_physical_device(session);
	create_logical_device(config, session);
	create_swapchain(session);
	create_image_views(session);
	create_render_pass(session);
	createDescriptorSetLayout(session);
	create_graphics_pipeline(session);
	create_command_pool(session);
	create_depth_resources(session);
	create_framebuffers(session); // after depth resources!
	create_texture_image(session);
	create_texture_image_view(session);
	create_texture_sampler(session);
	load_model(session);
	Buffer vertex_buffer = create_vertex_buffer(session->physical_device, session->logical_device, session->vertices, session->vertex_count, session->command_pool, session->graphics_queue);
	session->vertex_buffer = vertex_buffer.buffer;
	session->vertex_buffer_memory = vertex_buffer.memory;
	Buffer index_buffer = create_index_buffer(session->physical_device, session->logical_device, session->indices, session->index_count, session->command_pool, session->graphics_queue);
	session->index_buffer = index_buffer.buffer;
	session->index_buffer_memory = index_buffer.memory;
	create_uniform_buffers(session);
	create_descriptor_pool(session);
	create_descriptor_sets(session);
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
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroyBuffer(session->logical_device, session->uniform_buffers[i], 0);
		vkFreeMemory(session->logical_device, session->uniform_buffers_memory[i], 0);
	}
	vkDestroySampler(session->logical_device, session->sampler, 0);
	vkDestroyImageView(session->logical_device, session->texture_image_view, 0);
	vkDestroyImage(session->logical_device, session->texture_image, 0);
	vkFreeMemory(session->logical_device, session->texture_image_memory, 0);
	vkDestroyImageView(session->logical_device, session->depth_image_view, 0);
	vkDestroyImage(session->logical_device, session->depth_image, 0);
	vkFreeMemory(session->logical_device, session->depth_image_memory, 0);
	vkDestroyDescriptorPool(session->logical_device, session->descriptor_pool, 0);
	vkDestroyDescriptorSetLayout(session->logical_device, session->descriptor_set_layout, 0);
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

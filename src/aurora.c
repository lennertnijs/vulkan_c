#include "aurora.h"
#include "aurora_vulkan.h"
#include "aurora_tree.h"
#include "aurora_config.h"

#include <GLFW/glfw3.h>


struct AuroraSession {
	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue graphics_queue;
	uint32_t graphics_queue_index;
	VkQueue present_queue;
	uint32_t present_queue_index;
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
	Vertex *vertices;
	int vertex_count;
	uint16_t *indices;
	int index_count;
};

void window_resize_callback(GLFWwindow *window, int width, int height){
	width = width;
	height = height;
	resized = true;
}

void mouse_click_callback(GLFWwindow *window, int button, int action, int mods){
	if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT){	
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		// report back!
	}
}

void create_glfw_window(AuroraConfig *config, AuroraSession *session){
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
	glfwSetWindowUserPointer(window, session);
	glfwSetMouseButtonCallback(window, mouse_click_callback);
	glfwSetFramebufferSizeCallback(window, window_resize_callback);
	assert(window != NULL);
	session->window = window;
}

AuroraSession *aurora_session_create(AuroraConfig *config){
	AuroraSession *session = malloc(sizeof(AuroraSession));
	glfwInit();	
	create_glfw_window(config, session);
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	create_vk_instance(config->application_name, glfw_extensions, glfw_extension_count, config->enable_validation_layers, &session->instance);
	glfwCreateWindowSurface(session->instance, session->window, NULL, &session->surface);
	
	select_physical_device(session->instance, session->surface, &session->physical_device);	
	
	create_logical_device(session->physical_device, session->surface, config->allow_queue_sharing, config->enable_validation_layers, 
						  &session->graphics_queue_index, &session->present_queue_index, &session->device);	
	get_queues(session->device, session->graphics_queue_index, session->present_queue_index, &session->graphics_queue, &session->present_queue);	
	int width, height;
	glfwGetFramebufferSize(session->window, &width, &height);
	session->image_extent = (VkExtent2D){ .width = width, .height = height };
	create_swapchain(session->physical_device, session->surface, session->device, session->graphics_queue_index, session->present_queue_index, 
					  &session->image_count, &session->image_extent, &session->image_format, &session->swapchain);

    get_swapchain_images(session->device, session->swapchain, &session->images);
	
	// get the images
	create_image_views(session->image_count, session->images, session->image_format, &session->image_views);
	
	create_render_pass(session->device, session->image_format, &session->render_pass);
	
	init_graphics_pipeline(session);
	init_frame_buffers(session);
	init_command_pool(session);
	init_vertex_buffer(session);
	init_index_buffer(session);
	init_command_buffers(session);
	init_sync_objects(session);
	// create empty tree, or load old tree
}


void aurora_session_start(AuroraConfig *config, AuroraSession *session){
	while(!glfwWindowShouldClose(session->window)){
		glfwPollEvents();
		draw_frame();
	}	
}

void aurora_session_destroy(AuroraSession *session){
	// vulkan cleanup, then free the session
}




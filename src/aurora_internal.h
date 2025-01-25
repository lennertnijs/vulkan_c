#ifndef AURORA_INTERNAL_H
#define AURORA_INTERNAL_H

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include "aurora.h"

struct AuroraConfig{
	bool enable_validation_layers;
	bool allow_resize;
	int width;
	int height;
	char* application_name;
	char** vertex_shader;
	char** fragment_shader;
	Vertex *vertices;
	int vertex_count;
	uint16_t *indices;
	int index_count;
};

typedef struct {
	bool enable_validation_layers;
	char* application_name;
	uint32_t glfw_extension_count;
    const char** glfw_extensions;
	Vertex *vertices;
	int vertex_count;
	uint16_t *indices;
	int index_count;
} VkConfig;

typedef struct {
	GLFWwindow *window;
	VkInstance instance;
	VkSurfaceKHR surface;
	VkPhysicalDevice physical_device;
	uint32_t graphics_queue_index;
	uint32_t present_queue_index;
	VkDevice logical_device;
	VkQueue graphics_queue;
	VkQueue present_queue;
	VkSwapchainKHR swapchain;
	uint32_t image_count;
	VkSurfaceFormatKHR image_format;
	VkExtent2D image_extent;
	VkPresentModeKHR present_mode;
	VkSurfaceTransformFlagBitsKHR transform;
	VkImage *images;
	VkImageView *image_views;
	VkPipelineLayout pipeline_layout;
	VkRenderPass render_pass;
	VkPipeline graphics_pipeline;
	VkFramebuffer *frame_buffers;
	VkCommandPool command_pool;
	VkBuffer vertex_buffer;
	VkDeviceMemory vertex_buffer_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_buffer_memory;
	VkCommandBuffer *command_buffers;
	VkSemaphore *image_available_semaphores;
	VkSemaphore *render_finished_semaphores;
	VkFence *in_flight_fences;
	Vertex *vertices;
	int vertex_count;
	uint16_t *indices;
	int index_count;
} VkSession;

struct AuroraSession{
    VkConfig *vk_config;
    VkSession *vk_session;
};

#endif


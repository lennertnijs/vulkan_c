#ifndef BUFFER_H
#define BUFFER_H

#include "vulkan/vulkan.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "aurora.h"

typedef struct {
	VkBuffer buffer;
	VkDeviceMemory memory;
} Buffer;

extern Buffer create_vertex_buffer(
	VkPhysicalDevice phys_device,
	VkDevice device,
	Vertex* vertices,
	size_t count,
	VkCommandPool pool,
	VkQueue graphics_queue);

extern Buffer create_index_buffer(
	VkPhysicalDevice phys_device,
	VkDevice device,
	uint32_t* indices,
	uint32_t index_count,
	VkCommandPool pool,
	VkQueue graphics_queue);

#endif
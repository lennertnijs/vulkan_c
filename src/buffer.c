#include "buffer.h"

Buffer __create_buffer(
	VkPhysicalDevice phys_device,
	VkDevice device,
	VkDeviceSize size,
	VkBufferUsageFlags buffer_usage_flags,
	VkMemoryPropertyFlags memory_property_flags)
{
	Buffer buffer = { 0 };
	VkBufferCreateInfo info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = 0,
		.flags = 0,
		.size = size,
		.usage = buffer_usage_flags,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0, // not applicable when sharing mode is exclusive
		.pQueueFamilyIndices = 0
	};
	if (vkCreateBuffer(device, &info, 0, &buffer.buffer) != VK_SUCCESS) {
		printf("Could not create a VkBuffer.\n");
		exit(1);
	}

	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, buffer.buffer, &requirements);
	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = 0,
		.allocationSize = size,
		.memoryTypeIndex = _find_memory_type(phys_device, requirements.memoryTypeBits, memory_property_flags)
	};
	if (vkAllocateMemory(device, &alloc_info, 0, &buffer.memory) != VK_SUCCESS) {
		printf("Could not allocate VkDeviceMemory.\n");
		exit(1);
	}
	return buffer;
}

void _copy_data(
	VkDevice device,
	VkDeviceMemory memory,
	VkDeviceSize size,
	Vertex* vertices)
{
	void* data;
	vkMapMemory(device, memory, 0, size, 0, &data);
	memcpy(data, vertices, (size_t) size);
	vkUnmapMemory(device, memory);
}

uint32_t _find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
	for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}
	exit(1);
}

VkCommandBuffer _begin_single_time_commands(VkCommandPool pool, VkDevice device)
{
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = 0,
		.commandPool = pool,
		.commandBufferCount = 1,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
	};

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(command_buffer, &begin_info);
	return command_buffer;
}

void _end_single_time_commands(VkCommandPool pool, VkDevice device, VkQueue graphics_queue, VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);

	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &command_buffer
	};
	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);
	vkFreeCommandBuffers(device, pool, 1, &command_buffer);
}

void _copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkCommandPool pool, VkDevice device, VkQueue graphics_queue)
{
	VkCommandBuffer command_buffer = _begin_single_time_commands(pool, device);
	VkBufferCopy command = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &command);
	_end_single_time_commands(pool, device, graphics_queue, command_buffer);
}

Buffer create_vertex_buffer(
	VkPhysicalDevice phys_device,
	VkDevice device,
	Vertex* vertices,
	size_t count,
	VkCommandPool pool,
	VkQueue graphics_queue)
{
	VkDeviceSize size = sizeof(Vertex) * count;
	Buffer staging_buffer = __create_buffer(phys_device, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkBindBufferMemory(device, staging_buffer.buffer, staging_buffer.memory, 0);
	_copy_data(device, staging_buffer.memory, size, vertices);
	
	Buffer vertex_buffer = __create_buffer(phys_device, device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkBindBufferMemory(device, vertex_buffer.buffer, vertex_buffer.memory, 0);
	_copy_buffer(staging_buffer.buffer, vertex_buffer.buffer, size, pool, device, graphics_queue);
	
	vkDestroyBuffer(device, staging_buffer.buffer, 0);
	vkFreeMemory(device, staging_buffer.memory, 0);
	return vertex_buffer;
}

Buffer create_index_buffer(
	VkPhysicalDevice phys_device,
	VkDevice device,
	uint32_t* indices,
	uint32_t index_count,
	VkCommandPool pool,
	VkQueue graphics_queue)
{
	VkDeviceSize size = sizeof(uint32_t) * index_count;
	Buffer staging_buffer = __create_buffer(phys_device, device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkBindBufferMemory(device, staging_buffer.buffer, staging_buffer.memory, 0);
	void* data;
	vkMapMemory(device, staging_buffer.memory, 0, size, 0, &data);
	memcpy(data, indices, (size_t)size);
	vkUnmapMemory(device, staging_buffer.memory);

	Buffer index_buffer = __create_buffer(phys_device, device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkBindBufferMemory(device, index_buffer.buffer, index_buffer.memory, 0);
	_copy_buffer(staging_buffer.buffer, index_buffer.buffer, size, pool, device, graphics_queue);

	vkDestroyBuffer(device, staging_buffer.buffer, 0);
	vkFreeMemory(device, staging_buffer.memory, 0);
	return index_buffer;
}
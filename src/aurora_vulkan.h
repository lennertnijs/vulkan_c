#ifndef AURORA_VULKAN_H
#define AURORA_VULKAN_H

#include <vulkan/vulkan.h>

#include "aurora.h"

extern void 		create_vk_instance( const char* name, 
							   	  		const char** glfw_extensions, 
							      		int glfw_extension_count, 
							      		bool enable_validation, 
     							  		VkInstance *instance );
 
extern void 	select_physical_device( VkInstance instance, 
								  		VkSurfaceKHR surface, 
								   		VkPhysicalDevice *physical_device );

extern void 	 create_logical_device( VkPhysicalDevice physical_device, 
								  		VkSurfaceKHR surface, 
								  		bool allow_queue_sharing, 
								  		bool enable_validation,
							      		uint32_t *graphics_queue_index, 
								  		uint32_t *present_queue_index, 
								  		VkDevice *device );

extern void 		   		get_queues( VkDevice device, 
					  			  		uint32_t graphics_queue_index, 
					   	 		  		uint32_t present_queue_index, 
					   			  		VkQueue *graphics_queue, 
					   			 		VkQueue *present_queue );	


extern void 		  create_swapchain( VkPhysicalDevice physical_device, 
								 		VkSurfaceKHR surface,
										VkDevice device, 
								 		uint32_t graphics_queue_index, 
								 		uint32_t present_queue_index,
	 							 		uint32_t *image_count, 
								 		VkExtent2D *image_extent, 
								 		VkSurfaceFormatKHR *image_format, 
								 		VkSwapchainKHR *swapchain );

extern void 	  get_swapchain_images( VkDevice device, 
								 		VkSwapchainKHR swapchain, 
								 		VkImage **images );

extern void   		create_render_pass( VkDevice device, 
								 		VkSurfaceFormatKHR image_format, 
								 		VkRenderPass *render_pass );


extern void	init_graphics_pipeline(session);
extern void	init_frame_buffers(session);
extern void	init_command_pool(session);
extern void	init_vertex_buffer(session);
extern void	init_index_buffer(session);
extern void	init_command_buffers(session);
extern void	init_sync_objects(session);

#endif

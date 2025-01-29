#include "aurora_internal.h"
#include "aurora_vulkan.h"
#include "aurora_tree.c"

void window_resize_callback(GLFWwindow *window, int width, int height){
	width = width;
	height = height;
	// resized = true;
}

void mouse_click_callback(GLFWwindow *window, int button, int action, int mods){
	if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT){	
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		printf("aight you clicked.\n");
	}
}

void aurora_session_add_vertices(AuroraSession *session, Vertex *vertices, int vertex_count, uint16_t *indices, int index_count){
	add_vertices(session->vk_session, vertices, vertex_count, indices, index_count);
}

void aurora_session_start(AuroraConfig *config){
    glfwInit();
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
	VkConfig vkConfig = {
        .enable_validation_layers = config->enable_validation_layers,
        .application_name = config->application_name,
        .glfw_extension_count = glfw_extension_count,
        .glfw_extensions = glfw_extensions,
        .vertex_count = config->vertex_count,
        .vertices = config->vertices,
        .index_count = config->index_count,
        .indices = config->indices
    };
    VkSession *session = vulkan_session_create(&vkConfig);
    printf("Test succeeded\n");
	Tree *tree = create_tree(800, 600);
	glfwSetMouseButtonCallback(session->window, mouse_click_callback);
	glfwSetWindowUserPointer(session->window, session);
	glfwSetFramebufferSizeCallback(session->window, window_resize_callback);
	while(!glfwWindowShouldClose(vulkan_session_get_window(session))) {
        glfwPollEvents();
		vulkan_session_draw_frame(&vkConfig, session, false);
        // handle possible inputs -> tree
    }
    vulkan_session_destroy(session);
	glfwTerminate();
}
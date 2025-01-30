#include "aurora_internal.h"
#include "aurora_vulkan.h"
#include "aurora_tree.h"

void window_resize_callback(GLFWwindow *window, int width, int height){
    (void)window;
	width = width;
	height = height;
	// resized = true;
}

void mouse_click_callback(GLFWwindow *window, int button, int action, int mods){
    (void)mods;
	if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT){	
		double x, y;
		glfwGetCursorPos(window, &x, &y);
        AuroraSession *session = (AuroraSession*)glfwGetWindowUserPointer(window);
        split_node(session->tree, session->tree->root, (int)x, (int)y);
        size_t vertex_count;
        size_t index_count;
        Vertex *vertices;
        uint16_t *indices;
        get_draw_data(session->tree, &vertices, &vertex_count, &indices, &index_count);
        recreate_vertices(session->vk_session, vertices, vertex_count, indices, index_count);
	}
}

void aurora_session_start(AuroraConfig *config){
    glfwInit();
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    Tree *tree = create_tree(800, 600);
    size_t vertex_count;
    size_t index_count;
    Vertex *vertices;
    uint16_t *indices;
    get_draw_data(tree, &vertices, &vertex_count, &indices, &index_count);
	VkConfig vkConfig = {
        .enable_validation_layers = false,
        .application_name = config->application_name,
        .glfw_extension_count = glfw_extension_count,
        .glfw_extensions = glfw_extensions,
        .vertex_count = vertex_count,
        .vertices = vertices,
        .index_count = index_count,
        .indices = indices
    };
    VkSession *session = vulkan_session_create(&vkConfig);
    AuroraSession *aurora = malloc(sizeof(AuroraSession));
    aurora->vk_config = &vkConfig;
    aurora->vk_session = session;
    aurora->tree = tree;
    printf("Test succeeded\n");
	glfwSetMouseButtonCallback(session->window, mouse_click_callback);
	glfwSetWindowUserPointer(session->window, aurora);
	glfwSetFramebufferSizeCallback(session->window, window_resize_callback);
	while(!glfwWindowShouldClose(vulkan_session_get_window(session))) {
        glfwPollEvents();
		vulkan_session_draw_frame(session, false);
    }
    vulkan_session_destroy(session);
	glfwTerminate();
}
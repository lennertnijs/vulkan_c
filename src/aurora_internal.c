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
	//if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT){	
	//	double x, y;
	//	glfwGetCursorPos(window, &x, &y);
 //       AuroraSession *session = (AuroraSession*)glfwGetWindowUserPointer(window);
 //       split_node(session->tree, find_at(session->tree, (int)x, (int)y), (int)x, (int)y);
 //       size_t vertex_count;
 //       size_t index_count;
 //       Vertex *vertices;
 //       uint16_t *indices;
 //       get_draw_data(session->tree, &vertices, &vertex_count, &indices, &index_count);
 //       recreate_vertices(session->vk_session, vertices, vertex_count, indices, index_count);
	//}
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
    vertex_count = 4;
    index_count = 6;

    vertices = malloc(sizeof(Vertex) * vertex_count);
    indices = malloc(sizeof(uint16_t) * index_count);

    // Define rectangle vertices (positions in XY plane)
    vertices[0].position = (vec2s){ -0.5f, -0.5f };
    vertices[0].color = (vec3s){ 1.0f, 0.0f, 0.0f }; // Red - bottom left
    vertices[0].text_coord = (vec2s){ 1.0f, 0.0f };

    vertices[1].position = (vec2s){ 0.5f, -0.5f };
    vertices[1].color = (vec3s){ 0.0f, 1.0f, 0.0f }; // Green - bottom right
    vertices[1].text_coord = (vec2s){ 0.0f, 0.0f };

    vertices[2].position = (vec2s){ 0.5f, 0.5f };
    vertices[2].color = (vec3s){ 0.0f, 0.0f, 1.0f }; // Blue - top right
    vertices[2].text_coord = (vec2s){ 0.0f, 1.0f };

    vertices[3].position = (vec2s){ -0.5f, 0.5f };
    vertices[3].color = (vec3s){ 1.0f, 1.0f, 0.0f }; // Yellow - top left
    vertices[3].text_coord = (vec2s){ 1.0f, 1.0f };

    // Define indices for two triangles composing the rectangle
    indices[0] = 0; // first triangle
    indices[1] = 1;
    indices[2] = 2;

    indices[3] = 2; // second triangle
    indices[4] = 3;
    indices[5] = 0;
    //get_draw_data(tree, &vertices, &vertex_count, &indices, &index_count);
	VkConfig vkConfig = {
        .enable_validation_layers = true,
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
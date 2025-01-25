#include "aurora_internal.h"

VkSession *vkSession;

void window_resize_callback(GLFWwindow *window, int width, int height){
	width = width;
	height = height;
	// resized = true;
}

void mouse_click_callback(GLFWwindow *window, int button, int action, int mods){
	if(action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT){	
		double x, y;
		glfwGetCursorPos(window, &x, &y);
		// report back!
	}
}

	//if(config->allow_resize){
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	//}else{
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	//}
	//glfwSetWindowUserPointer(window, session);
	//glfwSetMouseButtonCallback(window, mouse_click_callback);
	//glfwSetFramebufferSizeCallback(window, window_resize_callback);

AuroraSession *aurora_session_create(AuroraConfig *config){
	glfwInit();
	VkConfig vkConfig = {};
	vkSession = create_vulkan_session(vkConfig);
}


void aurora_session_start(AuroraConfig *config, AuroraSession *session){
	while(!glfwWindowShouldClose(get_window(vkSession))) {
        glfwPollEvents();
		vulkan_session_draw_frame(config, vkSession, false);
    }
}

void aurora_session_destroy(AuroraSession *session){
	vulkan_session_destroy(vkSession);
	glfwTerminate();
}
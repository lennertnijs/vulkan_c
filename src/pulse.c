#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <GLFW/glfw3.h>

#include "pulse.h"

struct PulseSession {
	int width;
	int height;
	GLFWwindow *window;
	PulseResizeCallback resize_callback;
	PulseClickCallback click_callback;
	PulseKeyCallback key_callback;
};

void internal_resize_callback(GLFWwindow *window, int width, int height){
	PulseSession *session = (PulseSession*)glfwGetWindowUserPointer(window);
	session->resize_callback(width, height);
}

void internal_click_callback(GLFWwindow *window, int button, int action, int mods){
	PulseSession *session = (PulseSession*)glfwGetWindowUserPointer(window);
	double x;
	double y;
	glfwGetCursorPos(window, &x, &y);
	session->click_callback(x, y);
}

void internal_key_callback(GLFWwindow *window, int key, int super, int mods, int special){
	PulseSession *session = (PulseSession*)glfwGetWindowUserPointer(window);
	session->key_callback(A, PRESSED, CAPS);// translate, 
}

PulseSession *pulse_create(int width, int height, char *title,  bool allow_resize, 
					   PulseResizeCallback resize_callback, PulseClickCallback click_callback, PulseKeyCallback key_callback){
	if(!glfwInit()){
		printf("Could not initialise glfw.\n");
		abort();
	}
	PulseSession *session = malloc(sizeof(PulseSession));
	if(width < 0 || height < 0){
		printf("With and height have to be non-negative.\n");
		abort();
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, allow_resize);	
	session->width = width;
	session->height = height;
	session->window = glfwCreateWindow(session->width, session->height, title, NULL, NULL);
	if(session->resize_callback != NULL){
		glfwSetFramebufferSizeCallback(session->window, internal_resize_callback);
	}
	if(session->click_callback != NULL){
		glfwSetMouseButtonCallback(session->window, internal_click_callback);
	}
	if(session->key_callback != NULL){
		glfwSetKeyCallback(session->window, internal_key_callback);
	}
	while(!glfwWindowShouldClose(session->window)){
		glfwPollEvents();
	}
	glfwDestroyWindow(session->window);
	glfwTerminate();
	free(session);
}

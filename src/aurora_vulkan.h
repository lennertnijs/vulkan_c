#ifndef AURORA_VULKAN_H
#define AURORA_VULKAN_H

#include "aurora_internal.h"

extern VkSession *vulkan_session_create(VkConfig* config);
extern GLFWwindow *vulkan_session_get_window(VkSession *session);
extern void vulkan_session_draw_frame(VkSession *session, bool resized);
extern void vulkan_session_destroy(VkSession *session);
extern void recreate_vertices(VkSession *session, Vertex *vertices, int vertex_count, uint16_t *indices, int index_count);
#endif

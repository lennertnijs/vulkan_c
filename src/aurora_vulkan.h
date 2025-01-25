#ifndef AURORA_VULKAN_H
#define AURORA_VULKAN_H

#include "aurora_internal.h"

extern VkSession *vulkan_session_create(VkConfig* config);
extern GLFWwindow *vulkan_session_get_window(VkSession *session);
extern void vulkan_session_draw_frame(VkConfig *config, VkSession *session, bool resized);
extern void vulkan_session_destroy(VkSession *session);

#endif

#ifndef AURORA_H
#define AURORA_H

#include <stdint.h>
#include <stdbool.h>
#include<cglm/cglm.h>
#include <cglm/struct.h>

typedef struct {
	vec2s position;
	vec3s color;
} Vertex;

typedef struct AuroraConfig AuroraConfig;
typedef struct AuroraSession AuroraSession;


void mouse_clicked(AuroraSession *session, double x, double y);
extern AuroraConfig *aurora_config_create();

extern void aurora_config_destroy(AuroraConfig *config);
extern void aurora_config_set_window_size(AuroraConfig *config, int width, int height);
extern void aurora_config_set_window_allow_resize(AuroraConfig *config, bool allow_resize);
extern void aurora_config_enable_default_validation_layers(AuroraConfig *config);
extern void aurora_config_set_application_name(AuroraConfig *config, char *name);

extern void aurora_session_start(AuroraConfig *config);

#endif

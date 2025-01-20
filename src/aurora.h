#ifndef AURORA_H
#define AURORA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "vec.c"
typedef enum PresentMode {
	IMMEDIATE,
	MAILBOX, 
	FIFO,
	FIFO_RELAXED
} PresentMode;

typedef struct {
	Vec2 position;
	Vec3 color;
} Vertex;

typedef struct AuroraConfig AuroraConfig;
typedef struct AuroraSession AuroraSession;
typedef struct Node Node;


void mouse_clicked(AuroraSession *session, double x, double y);
extern AuroraConfig *aurora_config_create();
extern void aurora_config_destroy(AuroraConfig *config);

extern void aurora_config_set_window_size(AuroraConfig *config, int width, int height);
extern void aurora_config_set_window_allow_resize(AuroraConfig *config, bool allow_resize);

extern void aurora_config_enable_default_validation_layers(AuroraConfig *config);
extern void aurora_config_set_validation_layers(AuroraConfig *config, const char **names, int amount);
extern void aurora_config_set_application_name(AuroraConfig *config, char *name);
extern void aurora_config_set_application_version(AuroraConfig *config, int major, int minor, int patch, int subminor);
extern void aurora_config_set_engine_name(AuroraConfig *config, char *name);
extern void aurora_config_set_engine_version(AuroraConfig *config, int major, int minor, int patch, int subminor);
extern void aurora_config_set_api_version(AuroraConfig *config, int major, int minor, int patch, int subminor);

extern void aurora_config_enable_default_extensions(AuroraConfig *config);
extern void aurora_config_set_extensions(AuroraConfig *config, const char **names, int amount);

extern void aurora_config_allow_queue_sharing(AuroraConfig *config, bool allow_sharing);

extern void aurora_config_set_present_mode(AuroraConfig *config, PresentMode present_mode);
extern void aurora_config_set_image_count(AuroraConfig *config, int amount);


extern void aurora_config_set_vertices(AuroraConfig *config, Vertex *vertices, int amount, uint16_t *indices, int index_count);

extern AuroraSession *aurora_session_create(AuroraConfig *config);
extern void aurora_session_start(AuroraConfig *config, AuroraSession *session);
extern void aurora_session_destroy(AuroraSession *session);
extern void aurora_session_add_vertices(AuroraSession *session, Vertex *vertices, int vertex_count, uint16_t *indices, int index_count);


extern Node *create_tree(int width, int height, bool horizontal);
extern void split_node(Node *node, int x, int y);
#endif

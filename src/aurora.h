#ifndef AURORA_H
#define AURORA_H

#include <stdbool.h>
#include <stddef.h>


typedef struct AuroraConfig AuroraConfig;
typedef struct AuroraSession AuroraSession;

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

extern AuroraSession *aurora_session_create(AuroraConfig *config);
extern void aurora_session_destroy(AuroraSession *session);
#endif

#ifndef AURORA_H
#define AURORA_H

#include <stdbool.h>
#include <stddef.h>


typedef struct AuroraPreferences AuroraPreferences;
typedef struct AuroraSession AuroraSession;

extern AuroraPreferences *aurora_preferences_create();
extern void aurora_preferences_destroy(AuroraPreferences *preferences);

extern void aurora_preferences_set_window_size(AuroraPreferences *preferences, int width, int height);
extern void aurora_preferences_set_window_allow_resize(AuroraPreferences *preferences, bool allow_resize);

extern void aurora_preferences_enable_default_validation_layers(AuroraPreferences *preferences);
extern void aurora_preferences_set_validation_layers(AuroraPreferences *preferences, const char **names, int amount);
extern void aurora_preferences_set_application_name(AuroraPreferences *preferences, char *name);
extern void aurora_preferences_set_application_version(AuroraPreferences *preferences, int major, int minor, int patch, int subminor);
extern void aurora_preferences_set_engine_name(AuroraPreferences *preferences, char *name);
extern void aurora_preferences_set_engine_version(AuroraPreferences *preferences, int major, int minor, int patch, int subminor);
extern void aurora_preferences_set_api_version(AuroraPreferences *preferences, int major, int minor, int patch, int subminor);

extern void aurora_preferences_enable_default_extensions(AuroraPreferences *preferences);
extern void aurora_preferences_set_extensions(AuroraPreferences *preferences, const char **names, int amount);

extern AuroraSession *aurora_session_create(AuroraPreferences *preferences);
extern void aurora_session_destroy(AuroraSession *session);
#endif

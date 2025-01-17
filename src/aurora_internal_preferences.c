#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "aurora.h"
#include "aurora_internal_preferences.h"

const int default_validation_layer_count = 1;
const char *default_validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

const int default_extension_count = 1;
const char* default_extensions[] = {"VK_KHR_swapchain"};

AuroraPreferences *aurora_preferences_create(){
	AuroraPreferences *preferences = malloc(sizeof(AuroraPreferences));
    *preferences = (AuroraPreferences){
        .enable_validation_layers = false,
        .validation_layers = NULL,
		.validation_layer_count = 0,
        .allow_resize = false,
        .width = 800,
        .height = 600,
        .application_name = "Application name",
        .application_version = 4194304,
        .engine_name = "Engine name",
        .engine_version = 4194304,
        .api_version = 4194304,
		.extensions = NULL,
		.extension_count = 0,
        .graphics_queue_count = 0,
        .compute_queue_count = 0,
        .transfer_queue_count = 0,
        .allow_queue_sharing = false
    };
    return preferences;
}

void aurora_preferences_destroy(AuroraPreferences *preferences){
	free(preferences);
}

void aurora_preferences_set_window_size(AuroraPreferences *preferences, int width, int height){
	preferences->width = width;
	preferences->height = height;
}

void aurora_preferences_set_window_allow_resize(AuroraPreferences *preferences, bool allow_resize){
	preferences->allow_resize = allow_resize;
}

void aurora_preferences_enable_default_validation_layers(AuroraPreferences *preferences){
	preferences->validation_layers = default_validation_layers;
	preferences->validation_layer_count = default_validation_layer_count;
	preferences->enable_validation_layers = true;
}

void aurora_preferences_set_validation_layers(AuroraPreferences *preferences, const char **names, int amount){
	preferences->validation_layers = names;
	preferences->validation_layer_count = amount;
	preferences->enable_validation_layers = true;

}

void aurora_preferences_set_application_name(AuroraPreferences *preferences, char* name){
	preferences->application_name = name;
}

void aurora_preferences_set_application_version(AuroraPreferences *preferences, int major, int minor, int patch, int subminor){
	preferences->application_version = (((major) << 29) | ((minor) << 22) | ((patch) << 12) | (subminor));
}

void aurora_preferences_set_engine_name(AuroraPreferences *preferences, char* name){
	preferences->engine_name = name;
}

void aurora_preferences_set_engine_version(AuroraPreferences *preferences, int major, int minor, int patch, int subminor){
	preferences->engine_version = (((major) << 29) | ((minor) << 22) | ((patch) << 12) | (subminor));
}

void aurora_preferences_set_api_version(AuroraPreferences *preferences, int major, int minor, int patch, int subminor){
	preferences->api_version = (((major) << 29) | ((minor) << 22) | ((patch) << 12) | (subminor));
}

void aurora_preferences_enable_default_extensions(AuroraPreferences *preferences){
	preferences->extensions = default_extensions;
	preferences->extension_count = default_extension_count;
}

void aurora_preferences_set_extensions(AuroraPreferences *preferences, const char **names, int amount){
	preferences->extensions = names;
	preferences->extension_count = amount;
}

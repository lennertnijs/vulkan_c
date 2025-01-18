#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include "aurora.h"
#include "aurora_internal_config.h"

const int default_validation_layer_count = 1;
const char *default_validation_layers[] = {"VK_LAYER_KHRONOS_validation"};

const int default_extension_count = 1;
const char  *default_extensions[] = {"VK_KHR_swapchain"};

AuroraConfig *aurora_config_create(){
	AuroraConfig *config = malloc(sizeof(AuroraConfig));
    *config = (AuroraConfig){
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
        .graphics_queue_count = 1,
		.present_queue_count = 1,
        .compute_queue_count = 1,
        .transfer_queue_count = 1,
        .allow_queue_sharing = false
    };
    return config;
}

void aurora_config_destroy(AuroraConfig *config){
	free(config);
}

void aurora_config_set_window_size(AuroraConfig *config, int width, int height){
	config->width = width;
	config->height = height;
}

void aurora_config_set_window_allow_resize(AuroraConfig *config, bool allow_resize){
	config->allow_resize = allow_resize;
}

void aurora_config_enable_default_validation_layers(AuroraConfig *config){
	config->validation_layers = default_validation_layers;
	config->validation_layer_count = default_validation_layer_count;
	config->enable_validation_layers = true;
}

void aurora_config_set_validation_layers(AuroraConfig *config, const char **names, int amount){
	config->validation_layers = names;
	config->validation_layer_count = amount;
	config->enable_validation_layers = true;

}

void aurora_config_set_application_name(AuroraConfig *config, char* name){
	config->application_name = name;
}

void aurora_config_set_application_version(AuroraConfig *config, int major, int minor, int patch, int subminor){
	config->application_version = (((major) << 29) | ((minor) << 22) | ((patch) << 12) | (subminor));
}

void aurora_config_set_engine_name(AuroraConfig *config, char* name){
	config->engine_name = name;
}

void aurora_config_set_engine_version(AuroraConfig *config, int major, int minor, int patch, int subminor){
	config->engine_version = (((major) << 29) | ((minor) << 22) | ((patch) << 12) | (subminor));
}

void aurora_config_set_api_version(AuroraConfig *config, int major, int minor, int patch, int subminor){
	config->api_version = (((major) << 29) | ((minor) << 22) | ((patch) << 12) | (subminor));
}

void aurora_config_enable_default_extensions(AuroraConfig *config){
	config->extensions = default_extensions;
	config->extension_count = default_extension_count;
}

void aurora_config_set_extensions(AuroraConfig *config, const char **names, int amount){
	config->extensions = names;
	config->extension_count = amount;
}

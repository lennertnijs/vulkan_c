#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "aurora_internal.h"

AuroraConfig *aurora_config_create(){
	AuroraConfig *config = malloc(sizeof(AuroraConfig));
    *config = (AuroraConfig){
        .enable_validation_layers = false,
        .allow_resize = false,
        .width = 800,
        .height = 600,
        .application_name = "Application name",
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
	config->enable_validation_layers = true;
}

void aurora_config_set_application_name(AuroraConfig *config, char* name){
	config->application_name = name;
}

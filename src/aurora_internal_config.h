#ifndef AURORA_INTERNAL_CONFIG_H
#define AURORA_INTERNAL_CONFIG_H

#include <stdbool.h>

struct AuroraConfig {
	bool enable_validation_layers;
    const char** validation_layers;
	int validation_layer_count;	
	bool allow_resize;
	int width;
	int height;
	char* application_name;
	uint32_t application_version;
	char* engine_name;
	uint32_t engine_version;
	uint32_t api_version;
	const char** extensions;
	int extension_count;
	int graphics_queue_count;
	int present_queue_count;
	int compute_queue_count;
	int transfer_queue_count;
	bool allow_queue_sharing;	
};
#endif

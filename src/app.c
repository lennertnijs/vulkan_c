#include "aurora.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

int main(){
	AuroraConfig *config = aurora_config_create();
	aurora_config_set_window_size(config, 500, 200);
	aurora_config_enable_default_validation_layers(config);
	aurora_config_enable_default_extensions(config);
	aurora_config_set_graphics_queue_count(config, 1);
	aurora_config_set_present_queue_count(config, 1);
	aurora_config_allow_queue_sharing(config, true);

	AuroraSession *session = aurora_session_create(config);
	
	aurora_session_destroy(session);
	aurora_config_destroy(config);
	return 0;
}

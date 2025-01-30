#include <stdlib.h>
#include <stdio.h>

#include "aurora.h"



int main(){
	AuroraConfig *config = aurora_config_create();
	aurora_config_set_window_allow_resize(config, false);
	aurora_config_set_window_size(config, 800, 600);
	aurora_config_enable_default_validation_layers(config);
	aurora_session_start(config);
	aurora_config_destroy(config);
	return 0;
}


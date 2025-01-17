#include "aurora.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

int main(){
	AuroraPreferences *pref = aurora_preferences_create();
	aurora_preferences_set_window_size(pref, 500, 200);
	aurora_preferences_enable_default_validation_layers(pref);
	aurora_preferences_enable_default_extensions(pref);
	AuroraSession *session = aurora_session_create(pref);
	
	aurora_session_destroy(session);
	aurora_preferences_destroy(pref);
	return 0;
}

#include <stdlib.h>
#include <stdio.h>

#include "aurora.h"

int get_vertex_count(){
	return 4;
}

int get_index_count(){
	return 6;
}

uint16_t* get_indices(){
	uint16_t* indices = malloc(sizeof(uint16_t) * 6);
	indices[0] = 0;
	indices[1] = 1;
	indices[2] = 2;
	indices[3] = 2;
	indices[4] = 3;
	indices[5] = 0;
	return indices;	
}

Vertex* get_vertices(){
	Vertex *vertices = malloc(sizeof(Vertex) * get_vertex_count());
	vertices[0].position.x = -0.5f;
	vertices[0].position.y = -0.5f;
	vertices[0].color.x = 1.0f;
	vertices[0].color.y = 0.0f;
	vertices[0].color.z = 0.0f;

	vertices[1].position.x = 0.5f;
	vertices[1].position.y = -0.5f;
	vertices[1].color.x = 0.0f;
	vertices[1].color.y = 1.0f;
	vertices[1].color.z = 0.0f;

	vertices[2].position.x = 0.5f;
	vertices[2].position.y = 0.5f;
	vertices[2].color.x = 0.0f;
	vertices[2].color.y = 0.0f;
	vertices[2].color.z = 1.0f;
	
	vertices[3].position.x = -0.5f;
	vertices[3].position.y = 0.5f;
	vertices[3].color.x = 1.0f;
	vertices[3].color.y = 1.0f;
	vertices[3].color.z = 1.0f;
	return vertices;
}



int main(){
	AuroraConfig *config = aurora_config_create();
	aurora_config_set_window_allow_resize(config, true);
	aurora_config_set_window_size(config, 800, 600);
	aurora_config_enable_default_validation_layers(config);
	aurora_config_enable_default_extensions(config);
	aurora_config_allow_queue_sharing(config, true);
	aurora_config_set_vertices(config, get_vertices(), 4, get_indices(), 6);
	AuroraSession *session = aurora_session_create(config);

	aurora_session_start(config, session);
	
	aurora_session_destroy(session);
	aurora_config_destroy(config);
	
	return 0;
}


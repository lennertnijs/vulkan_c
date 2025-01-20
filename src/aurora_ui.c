#include "aurora.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "aurora_internal.h"

typedef enum Rotation {
	HORIZONTAL,
	VERTICAL
} Rotation;

struct Node {
	int x;
	int y;
	int width;
	int height;
	Rotation rotation;
	Node *parent;
	Node **children;
	int child_count;
	int capacity;
};

bool is_within(Node *outside, Node *inside){
	return outside->x <= inside->x && outside->y <= inside->y && outside->x + outside->width >= inside->x + inside->width && outside->y + outside->height >= inside->y + inside->height;
}

void add_child(Node *parent, Node *child){
	if(parent == NULL){
		printf("Cannot add a child Node to NULL parent.\n");
		return;
	}
	if(child == NULL){
		printf("Cannot add a NULL child Node to a parent Node.\n");
		return;
	}
	if(!is_within(parent, child)){
		printf("The child Node is not fully contained by the parent Node.");
		return;
	}
	if(parent->child_count < parent->capacity){
		parent->children[parent->child_count++] = child;
	}else{
		parent->capacity *= 2;
		Node** children = parent->children;
		parent->children = malloc(sizeof(Node) * parent->capacity);
		parent->children = memcpy(parent->children, children, sizeof(Node*) * parent->child_count);
		free(children);
		parent->children[parent->child_count++] = child;
	}
}

void mouse_clicked(AuroraSession *session, double x, double y){
//	printf("The following was clicked: %f, %f\n", x, y);
	Vertex *vertices = malloc(sizeof(Vertex) * 4);
	vertices[0] = (Vertex){
		.position.x = 0.9f,
		.position.y = 0.7f,
		.color.x = 1.0f,
		.color.y = 1.0f,
		.color.z = 0.0f
	};
	
	vertices[1] = (Vertex){
		.position.x = 0.9f,
		.position.y = 0.9f,
		.color.x = 1.0f,
		.color.y = 1.0f,
		.color.z = 0.0f
	};
	vertices[2] = (Vertex){
		.position.x = 0.7f,
		.position.y = 0.9f,
		.color.x = 1.0f,
		.color.y = 1.0f,
		.color.z = 0.0f
	};
	vertices[3] = (Vertex){	
		.position.x = 0.7f,
		.position.y = 0.7f,
		.color.x = 1.0f,
		.color.y = 0.0f,
		.color.z = 1.0f
	};
	uint16_t *indices = malloc(sizeof(uint16_t) * 6);
	indices[0] = 4;
	indices[1] = 5;
	indices[2] = 6;
	indices[3] = 6;
	indices[4] = 7;
	indices[5] = 4;
	aurora_session_add_vertices(session, vertices, 6, indices, 6);
}

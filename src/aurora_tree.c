#include "aurora.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum Rotation {
	HORIZONTAL,
	VERTICAL
};

struct Node {
	int x;
	int y;
	int width;
	int height;
	bool horizontal;
	Node *parent;
	Node **children;
	int child_count;
	int capacity;
};

const int default_capacity = 8;

Node *create_tree(int width, int height, bool horizontal){
	Node *node = malloc(sizeof(Node));
	*node = (Node){
		.x = 0,
		.y = 0,
		.width = width,
		.height = height,
		.horizontal = horizontal,
		.parent = NULL,
		.children = NULL,
		.child_count = 0,
		.capacity = default_capacity
	};
	return node;
}

bool is_inside_node(Node *outside, Node *inside){
	return outside->x <= inside->x && outside->y <= inside->y && outside->x + outside->width >= inside->x + inside->width && outside->y + outside->height >= inside->y + inside->height;
}
bool is_within(Node *node, int x, int y){
	return node->x <= x && node->y <= y && node->x + node->width >= x && node->y + node->height >= y;
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
	if(parent->children == NULL){
		parent->children = malloc(sizeof(Node) * parent->capacity);
	}
	if(!is_inside_node(parent, child)){
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

void split_node(Node *parent, double x, double y){
	Node *clicked;
	for(int i = 0; i < parent->child_count; i++){
		if(is_within(parent->children[i], (int) x, (int) y)){
			clicked = parent->children[i];
		}
	}
	Node *new = malloc(sizeof(Node));
	if(parent->horizontal){
		new->x = clicked->x;
		new->y = (int)y;
		clicked->height = (int)y - clicked->y; 
		new->height = clicked->height + clicked->y - (int)y;
		new->width = clicked->width;
		new->horizontal = true;
	}else{
		new->x = (int)x;
		new->y = clicked->y;
		clicked->width = (int)x - clicked->x;
		new->height = clicked->height + clicked->y - (int)y;
		new->width = clicked->width + clicked->x - (int)x;
		new->horizontal = false;
	}
	new->parent = parent;
	add_child(parent, new);
}

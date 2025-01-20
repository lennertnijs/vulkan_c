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

// destroy the tree, dawg

void split_node(Node *node, int x, int y){
	// check is within
	
}

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
	if(parent->children == NULL){
		parent->children = malloc(sizeof(Node) * parent->capacity);
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

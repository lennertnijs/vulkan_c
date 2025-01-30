#ifndef AURORA_TREE_H
#define AURORA_TREE_H
#include "aurora.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct Node Node;

typedef enum Rotation {
    HORIZONTAL,
    VERTICAL
} Rotation;

struct Node {
    int x, y;
    int width, height;
    Node *parent;
    Node **children;
    int child_count;
};

typedef struct{
    Node *root;
    int node_count;
    int width;
    int height;
} Tree;

extern Tree *create_tree(int width, int height);
extern void split_node(Tree *tree, Node *current, int x, int y);
extern void get_draw_data(Tree *tree, Vertex **vertices, size_t *vertex_count, uint16_t **indices, size_t *index_count);

#endif
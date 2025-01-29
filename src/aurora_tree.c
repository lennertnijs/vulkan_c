#include <stdlib.h>

#include "aurora_internal.h"


typedef struct Node Node;

const int capacity = 10;

typedef struct {
    Node *root;
    int node_count;
} Tree;

struct Node {
    int x, y;
    int width, height;
    Node *parent;
    Node **children;
    int child_count;
};

Tree *create_tree(int width, int height){
    Node *node = malloc(sizeof(Node));
    node->x = 0;
    node->y = 0;
    node->width = width;
    node->height = height;
    node->parent = NULL;
    node->children = malloc(sizeof(Node) * capacity);
    node->child_count = 0;
    Tree *tree = malloc(sizeof(Tree));
    tree->node_count = 1;
    tree->root = node;
    return tree;
}

Vertex* transform_to_vertices(Tree* tree){
    int leaf_count = tree->node_count;
    Vertex *vertices = malloc(sizeof(Vertex) * leaf_count);
    uint32_t index = 0;
    create_vertices_of_tree(tree->root, &vertices, &index);
}

Vertex* translate_node_to_vertices(Node *node){
    Vertex* vertices = malloc(sizeof(Vertex) * 6);
    Vec3 color = {.x = 1.0, .y = 1.0, .z = 1.0};
    Vertex v1 = { .position = {.x = node->x, .y=node->y}, .color = color};
    Vertex v2 = { .position = {.x = node->x + node->width, .y=node->y}, .color = color};
    Vertex v3 = { .position = {.x = node->x, .y=node->y + node->height}, .color = color};
    Vertex v4 = { .position = {.x = node->x + node->width, .y=node->y + node->height}, .color = color};
    vertices[0] = v1;
    vertices[1] = v2;
    vertices[2] = v3;
    vertices[3] = v4;
    vertices[4] = v3;
    vertices[5] = v2;
    return vertices;
}   

void create_vertices_of_tree(Node *current, Vertex** vertices, uint32_t *current_index){
    if(current->child_count == 0){
        for(int i = 0; i < 6; i++){
            (*vertices)[*current_index + i] = translate_node_to_vertices(current)[i];
        }
    }
    for(int i = 0; i < current->child_count; i++){
        create_vertices_of_tree(current->children[i], &vertices, &current_index);
    }
}
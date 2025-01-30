#include <stdlib.h>

#include "aurora_tree.h"

const int capacity = 10;

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
    tree->width = width;
    tree->height = height;
    tree->node_count = 1;
    tree->root = node;
    return tree;
}

void print_node(Node *node){
    printf("Printing a Node: \n");
    printf("The position: (x, y) = (%d, %d)\n", node->x, node->y);
    printf("The dimensions: (width, height) = (%d, %d)\n", node->width, node->height);
    printf("The child count: %d\n", node->child_count);
    for(int i = 0; i < node->child_count; i++){
        printf("Child %d: %p |", i, node->children[i]);
    }
    printf("\n");
}

void split_node(Tree *tree, Node *current, int x, int y){
    Node *left = malloc(sizeof(Node));
    Node *right = malloc(sizeof(Node));
    left->x = current->x;
    left->y = current->y;
    left->width = x - current->x;
    left->height = current->height;
    left->child_count = 0;
    right->x = x;
    right->y = current->y;
    right->width = current->width - current->x - x;
    right->height = current->height;
    right->child_count = 0;
    current->children[(current->child_count)++] = left;
    current->children[(current->child_count)++] = right;
    tree->node_count += 1;
}

float translate_to_screenspace(int number, int width, int height, Rotation rotation){
    switch(rotation){
        case HORIZONTAL:
            return 2.0f * ((float)number / (float)width) - 1.0f;
        case VERTICAL:
            return 2.0f * ((float)number / (float)height) - 1.0f;
        default:
            printf("Error");
            abort();
    }
}

void translate(Node *current, Vertex **vertices, size_t *vertex_index, uint16_t **indices, size_t *index_index, int w, int h){
    if(current->child_count == 0){
        Vec3 red = { .x = 1.0f, .y = 0.0f, .z = 0.0f};
        Vec3 green = { .x = 0.0f, .y = 1.0f, .z = 0.0f};
        Vec3 blue = { .x= 0.0f, .y = 0.0f, .z = 1.0f};
        Vec3 yellow = { .x = 1.0f, .y = 1.0f, .z = 0.0f};
        Vertex top_left = {
            .position.x = translate_to_screenspace(current->x, w, h, HORIZONTAL),
            .position.y = translate_to_screenspace(current->y, w, h, VERTICAL),
            .color = red
        };
        Vertex top_right = {
              .position.x = translate_to_screenspace(current->x + current->width, w, h, HORIZONTAL),
              .position.y = translate_to_screenspace(current->y, w, h, VERTICAL),
              .color = green
        };
        Vertex bottom_left = {
              .position.x = translate_to_screenspace(current->x, w, h, HORIZONTAL),
              .position.y = translate_to_screenspace(current->y + current->height, w, h, VERTICAL),
              .color = blue
        };
        Vertex bottom_right = {
              .position.x = translate_to_screenspace(current->x + current->width, w, h, HORIZONTAL),
              .position.y = translate_to_screenspace(current->y + current->height, w, h, VERTICAL),
              .color = yellow
        };
        size_t vertex_start_index = *vertex_index;
        (*vertices)[(*vertex_index)++] = top_left;
        (*vertices)[(*vertex_index)++] = top_right;
        (*vertices)[(*vertex_index)++] = bottom_left;
        (*vertices)[(*vertex_index)++] = bottom_right;
        (*indices)[(*index_index)++] = vertex_start_index;
        (*indices)[(*index_index)++] = vertex_start_index + 1;
        (*indices)[(*index_index)++] = vertex_start_index + 2;
        (*indices)[(*index_index)++] = vertex_start_index + 2;
        (*indices)[(*index_index)++] = vertex_start_index + 3;
        (*indices)[(*index_index)++] = vertex_start_index + 1;
    }else{
        for(int i = 0; i < current->child_count; i++){
            translate(current->children[i], vertices, vertex_index, indices, index_index, w, h);
        }
    }
}

void get_draw_data(Tree *tree, Vertex **vertices, size_t *vertex_count, uint16_t **indices, size_t *index_count){
    *vertex_count = tree->node_count * 4;
    *index_count = tree->node_count * 6;
    *vertices = malloc(sizeof(Vertex) * (*vertex_count));
    *indices = malloc(sizeof(uint16_t) * (*index_count));
    size_t vertex_index = 0;
    size_t index_index = 0;
    translate(tree->root, vertices, &vertex_index, indices, &index_index, tree->width, tree->height);
}
#include <stdlib.h>

#include "aurora_tree.h"

const int capacity = 10;

static Node* create_node(int x, int y, int width, int height, Node* parent, Node** children, size_t child_count) {
    Node* node = malloc(sizeof(Node));
    if (node == 0) { abort(); };
    node->x = x;
    node->y = y;
    node->width = width;
    node->height = height;
    node->parent = parent;
    node->children = children;
    node->child_count = child_count;
    node->child_capacity = child_count != 0 ? 2 * child_count : capacity;
    return node;
}

Tree *create_tree(int width, int height){
    Node* node = create_node(0, 0, width, height, NULL, malloc(sizeof(Node *) * capacity), 0);
    Tree *tree = malloc(sizeof(Tree));
    if (tree == 0) { abort(); }
    tree->width = width;
    tree->height = height;
    tree->node_count = 1;
    tree->root = node;
    tree->rotation = VERTICAL;
    return tree;
}

bool contains(Node *node, int x, int y){
    return node->x <= x && node->y <= y && node->x + node->width >= x && node->y + node->height >= y;
}

void insert_child(Node *parent, Node *child){
    for(size_t i = 0; i < parent->child_capacity; i++){
        if(parent->children[i] == NULL){
            parent->children[i] = child;
            return;
        }
    }
    // no slot found
    Node** nodes = malloc(sizeof(Node*) * parent->child_capacity * 2);
    if (nodes == 0) { abort(); }
    Node** old_nodes = parent->children;
    parent->children = nodes;
    for (size_t i = 0; i < parent->child_count; i++) {
        parent->children[i] = old_nodes[i];
    }
    parent->children[parent->child_count] = child;
    parent->child_count += 1;
    free(old_nodes);
}

size_t remove_child(Node* parent, Node* child){
    for(size_t i = 0; i < parent->child_capacity; i++){
        if(parent->children[i] == child){
            free(child);
            parent->children[i] = NULL;
            return i;
        }
    }
    return 0; // t odo ffix
}

Node* find_at_recursive(Node *node, int x, int y){
    if (!contains(node, x, y)) return NULL;
    if (node->child_count == 0) return node;
    for (size_t i = 0; i < node->child_count; i++) {
        if (!contains(node->children[i], x, y)) continue;
        return find_at_recursive(node->children[i], x, y);
    }
    return NULL;
}

void print_node(Node *node){
    printf("Printing a Node: \n");
    printf("The position: (x, y) = (%d, %d)\n", node->x, node->y);
    printf("The dimensions: (width, height) = (%d, %d)\n", node->width, node->height);
    printf("The child count: %d\n", node->child_count);
    for(size_t i = 0; i < node->child_count; i++){
        print_node(node->children[i]);
    }
    printf("\n");
}

Node* find_at(Tree* tree, int x, int y) {
    return find_at_recursive(tree->root, x, y);
}

void split_node(Tree *tree, Node *current, int x, int y){
    Node* left = create_node(current->x, current->y, x - current->x, current->height, NULL, malloc(sizeof(Node *) * capacity), 0);
    Node *right = create_node(x, current->y, current->width - x + current->x, current->height, NULL, malloc(sizeof(Node*) * capacity), 0);
    print_node(current);
    if(current->parent != NULL){
        int removed_index = remove_child(current->parent, current);
        current->parent->children[removed_index] = left;
        left->parent = current->parent;
        current->parent->children[(current->parent->child_count)++] = right;
        right->parent = current->parent;
    }else{
        insert_child(current, left);
        insert_child(current, right);
    }
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
        vec3s red = { .x = 1.0f, .y = 0.0f, .z = 0.0f};
        vec3s green = { .x = 0.0f, .y = 1.0f, .z = 0.0f};
        vec3s blue = { .x= 0.0f, .y = 0.0f, .z = 1.0f};
        vec3s yellow = { .x = 1.0f, .y = 1.0f, .z = 0.0f};
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
        for(size_t i = 0; i < current->child_count; i++){
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
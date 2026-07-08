#ifndef TREE_H
#define TREE_H

#define MAX_LEN 256

typedef struct Node {
    char text[MAX_LEN];   // question text OR answer (object name)
    int is_leaf;          // 1 if this is an answer node, 0 if question
    struct Node *yes;
    struct Node *no;
} Node;

Node* create_node(const char *text, int is_leaf);
void free_tree(Node *root);

Node* load_tree(const char *filename);
void save_tree(const char *filename, Node *root);

void print_tree(Node *root, int depth);

#endif

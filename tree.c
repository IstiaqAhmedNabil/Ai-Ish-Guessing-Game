#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tree.h"

/*
 * Node creation
 */
Node *create_node(const char *text, int is_leaf)
{
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n)
    {
        fprintf(stderr, "Memory allocation failed!\n");
        exit(1);
    }
    strncpy(n->text, text, MAX_LEN - 1);
    n->text[MAX_LEN - 1] = '\0';
    n->is_leaf = is_leaf;
    n->yes = NULL;
    n->no = NULL;
    return n;
}

/*
 * Recursively free the entire tree
 */
void free_tree(Node *root)
{
    if (root == NULL)
        return;
    free_tree(root->yes);
    free_tree(root->no);
    free(root);
}

/*
 * Helper: strip trailing newline from fgets result
 */
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
    {
        s[len - 1] = '\0';
        len--;
    }
}

/*
 * --- File format ---
 * The tree is stored using a simple pre-order (root, yes-subtree, no-subtree)
 * serialization. Each node is written as one line:
 *
 *   Q:<question text>     -> a question node (internal node)
 *   A:<answer text>       -> a leaf / answer node
 *   #                      -> represents a NULL child (empty subtree)
 *
 * Pre-order traversal means: for every question node we recursively
 * write its YES subtree first, then its NO subtree. Leaves and '#'
 * markers have no children, so nothing further is written for them.
 */

static void save_node(FILE *fp, Node *node)
{
    if (node == NULL)
    {
        fprintf(fp, "#\n");
        return;
    }
    if (node->is_leaf)
    {
        fprintf(fp, "A:%s\n", node->text);
    }
    else
    {
        fprintf(fp, "Q:%s\n", node->text);
        save_node(fp, node->yes);
        save_node(fp, node->no);
    }
}

void save_tree(const char *filename, Node *root)
{
    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        fprintf(stderr, "Error: could not open '%s' for writing.\n", filename);
        return;
    }
    save_node(fp, root);
    fclose(fp);
}

static Node *load_node(FILE *fp)
{
    char line[MAX_LEN];
    if (fgets(line, MAX_LEN, fp) == NULL)
    {
        return NULL; /* unexpected EOF -> treat as empty */
    }
    strip_newline(line);

    if (strcmp(line, "#") == 0)
    {
        return NULL;
    }
    else if (line[0] == 'Q' && line[1] == ':')
    {
        Node *n = create_node(line + 2, 0);
        n->yes = load_node(fp);
        n->no = load_node(fp);
        return n;
    }
    else if (line[0] == 'A' && line[1] == ':')
    {
        Node *n = create_node(line + 2, 1);
        return n;
    }
    /* Unknown line format - skip and try next */
    return load_node(fp);
}

Node *load_tree(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        return NULL; /* file doesn't exist yet */
    }
    Node *root = load_node(fp);
    fclose(fp);
    return root;
}

/*
 * Print the tree to console for visualization.
 * Question nodes show their question; branches are labeled YES/NO.
 * Leaf nodes show the guessed answer.
 */
void print_tree(Node *root, int depth)
{
    if (root == NULL)
        return;

    for (int i = 0; i < depth; i++)
        printf("    ");

    if (root->is_leaf)
    {
        printf("- [Guess] %s\n", root->text);
    }
    else
    {
        printf("- [Q] %s\n", root->text);
        for (int i = 0; i < depth; i++)
            printf("    ");
        printf("  YES ->\n");
        print_tree(root->yes, depth + 2);
        for (int i = 0; i < depth; i++)
            printf("    ");
        printf("  NO ->\n");
        print_tree(root->no, depth + 2);
    }
}

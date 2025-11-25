#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node
{
    char text[100];
    struct Node *yes;
    struct Node *no;
} Node;

Node *makeNode(char *str)
{
    Node *n = (Node *)malloc(sizeof(Node));
    strcpy(n->text, str);
    n->yes = NULL;
    n->no = NULL;
    return n;
}

void play(Node *current)
{
    char answer;

    if (current->yes == NULL && current->no == NULL)
    {
        printf("Is it a %s? (y/n): ", current->text);
        scanf(" %c", &answer);

        if (answer == 'y')
        {
            printf("I got it right!\n");
        }
        else
        {
            char newThing[100], question[100];

            printf("What was it? ");
            scanf(" %[^\n]", newThing);

            printf("Give me a question to tell '%s' from '%s'\n",
                   newThing, current->text);
            printf("Question: ");
            scanf(" %[^\n]", question);

            printf("For '%s', answer is y or n? ", newThing);
            scanf(" %c", &answer);

            Node *newNode = makeNode(newThing);
            Node *oldNode = makeNode(current->text);

            strcpy(current->text, question);

            if (answer == 'y')
            {
                current->yes = newNode;
                current->no = oldNode;
            }
            else
            {
                current->yes = oldNode;
                current->no = newNode;
            }

            printf("Thanks! I learned something.\n");
        }
        return;
    }

    printf("%s (y/n): ", current->text);
    scanf(" %c", &answer);

    if (answer == 'y')
    {
        play(current->yes);
    }
    else
    {
        play(current->no);
    }
}

void save(Node *root, FILE *f)
{
    if (root == NULL)
    {
        fprintf(f, "#\n");
        return;
    }
    fprintf(f, "%s\n", root->text);
    save(root->yes, f);
    save(root->no, f);
}

Node *load(FILE *f)
{
    char line[100];

    if (fgets(line, 100, f) == NULL)
    {
        return NULL;
    }

    line[strlen(line) - 1] = '\0';

    if (strcmp(line, "#") == 0)
    {
        return NULL;
    }

    Node *n = makeNode(line);
    n->yes = load(f);
    n->no = load(f);
    return n;
}

void cleanup(Node *root)
{
    if (root == NULL)
        return;
    cleanup(root->yes);
    cleanup(root->no);
    free(root);
}

int main()
{
    Node *root = NULL;
    FILE *f;
    char again;

    printf("AI Guessing Game\n");
    printf("By: Istiaq Ahmed Nabil\n");
    printf("ID: 20254103139\n\n");

    f = fopen("game.txt", "r");
    if (f != NULL)
    {
        root = load(f);
        fclose(f);
        printf("Loaded previous game\n\n");
    }
    else
    {
        root = makeNode("dog");
        printf("Starting new game\n\n");
    }

    do
    {
        printf("Think of something!\n");
        play(root);

        printf("\nPlay again? (y/n): ");
        scanf(" %c", &again);
        printf("\n");
    } while (again == 'y');

    f = fopen("game.txt", "w");
    if (f != NULL)
    {
        save(root, f);
        fclose(f);
    }

    cleanup(root);
    printf("Bye!\n");

    return 0;
}
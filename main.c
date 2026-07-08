#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tree.h"

#define TREE_FILE   "tree.dat"
#define STATS_FILE  "stats.dat"

/* ---------------------------------------------------------
 * Global state
 * --------------------------------------------------------- */
static Node *root = NULL;

static int stat_correct = 0;
static int stat_wrong   = 0;

/* For "undo last learning" - we remember the single node that was
 * converted from a leaf into a question node during the most recent
 * learning session, along with its original (pre-learning) state,
 * so we can revert it back to a plain leaf and discard the two new
 * children that were attached to it. */
static Node *last_modified_node = NULL;
static char  last_modified_old_text[MAX_LEN];
static int   undo_available = 0;

/* ---------------------------------------------------------
 * Input helpers
 * --------------------------------------------------------- */

/* Read a full line of input, stripping the trailing newline. */
static void read_line(char *buffer, int size) {
    if (fgets(buffer, size, stdin) == NULL) {
        /* EOF or input error - exit cleanly rather than looping forever */
        printf("\nInput ended. Exiting.\n");
        exit(0);
    }
    size_t len = strlen(buffer);
    while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
        buffer[len-1] = '\0';
        len--;
    }
}

/* Ask a yes/no question until the user gives a valid answer.
 * Accepts: yes/y/no/n (case-insensitive). Returns 1 for yes, 0 for no. */
static int ask_yes_no(const char *question) {
    char buf[MAX_LEN];
    while (1) {
        printf("%s (yes/no): ", question);
        read_line(buf, sizeof(buf));

        /* lowercase the whole response for easy comparison */
        for (int i = 0; buf[i]; i++) buf[i] = (char) tolower((unsigned char) buf[i]);

        if (strcmp(buf, "yes") == 0 || strcmp(buf, "y") == 0) return 1;
        if (strcmp(buf, "no")  == 0 || strcmp(buf, "n") == 0) return 0;

        printf("Please answer with 'yes' or 'no'.\n");
    }
}

/* Ask for a free-text answer, re-prompting if the user enters nothing. */
static void ask_text(const char *prompt, char *out, int size) {
    while (1) {
        printf("%s", prompt);
        read_line(out, size);
        if (strlen(out) > 0) return;
        printf("Please enter a non-empty answer.\n");
    }
}

/* ---------------------------------------------------------
 * Statistics
 * --------------------------------------------------------- */

static void load_stats(void) {
    FILE *fp = fopen(STATS_FILE, "r");
    if (!fp) {
        stat_correct = 0;
        stat_wrong = 0;
        return;
    }
    if (fscanf(fp, "%d %d", &stat_correct, &stat_wrong) != 2) {
        stat_correct = 0;
        stat_wrong = 0;
    }
    fclose(fp);
}

static void save_stats(void) {
    FILE *fp = fopen(STATS_FILE, "w");
    if (!fp) {
        fprintf(stderr, "Warning: could not save statistics.\n");
        return;
    }
    fprintf(fp, "%d %d\n", stat_correct, stat_wrong);
    fclose(fp);
}

static void show_stats(void) {
    int total = stat_correct + stat_wrong;
    printf("\n----- Statistics -----\n");
    printf("Correct guesses : %d\n", stat_correct);
    printf("Wrong guesses   : %d\n", stat_wrong);
    if (total > 0) {
        double pct = (100.0 * stat_correct) / total;
        printf("Total games     : %d\n", total);
        printf("Accuracy        : %.1f%%\n", pct);
    } else {
        printf("Total games     : 0\n");
    }
    printf("-----------------------\n\n");
}

/* ---------------------------------------------------------
 * Default knowledge base
 * --------------------------------------------------------- */

/* Starting tree: a single leaf. The AI's first ever guess will simply
 * be "Dog" -- it has no questions yet. Every time it is wrong, the
 * learning module will grow the tree with new questions and answers. */
static Node* build_default_tree(void) {
    return create_node("Dog", 1);
}

/* ---------------------------------------------------------
 * Learning module
 * --------------------------------------------------------- */

/* Called when the AI's guess (stored in the leaf `wrong_leaf`) was
 * incorrect. Converts that leaf into a new question node with two
 * leaf children: one for the AI's old (wrong) guess, and one for the
 * new object the user was thinking of. */
static void learn_new_object(Node *wrong_leaf) {
    char correct_answer[MAX_LEN];
    char new_question[MAX_LEN];
    char old_guess[MAX_LEN];

    printf("\nAI: I failed! What were you thinking of?\n");
    ask_text("You: ", correct_answer, sizeof(correct_answer));

    printf("\nAI: Give me a yes/no question that distinguishes a '%s' from a '%s':\n",
           correct_answer, wrong_leaf->text);
    ask_text("You: ", new_question, sizeof(new_question));

    printf("\nAI: For a '%s', what is the answer to that question?\n", correct_answer);
    int answer_for_new = ask_yes_no("So");

    /* Save the old guess text before we overwrite the node */
    strncpy(old_guess, wrong_leaf->text, MAX_LEN - 1);
    old_guess[MAX_LEN - 1] = '\0';

    /* Remember pre-learning state for "undo" */
    last_modified_node = wrong_leaf;
    strncpy(last_modified_old_text, old_guess, MAX_LEN - 1);
    last_modified_old_text[MAX_LEN - 1] = '\0';
    undo_available = 1;

    /* Convert the leaf into a question node */
    strncpy(wrong_leaf->text, new_question, MAX_LEN - 1);
    wrong_leaf->text[MAX_LEN - 1] = '\0';
    wrong_leaf->is_leaf = 0;

    Node *new_obj_node = create_node(correct_answer, 1);
    Node *old_guess_node = create_node(old_guess, 1);

    if (answer_for_new) {
        /* New object's answer is YES -> goes in the yes branch */
        wrong_leaf->yes = new_obj_node;
        wrong_leaf->no  = old_guess_node;
    } else {
        wrong_leaf->yes = old_guess_node;
        wrong_leaf->no  = new_obj_node;
    }

    printf("\nAI: Learned! Thank you.\n\n");
}

/* ---------------------------------------------------------
 * Undo last learning
 * --------------------------------------------------------- */

static void undo_last_learning(void) {
    if (!undo_available || last_modified_node == NULL) {
        printf("\nNothing to undo.\n\n");
        return;
    }

    /* Free the two children that were added during learning */
    free_tree(last_modified_node->yes);
    free_tree(last_modified_node->no);
    last_modified_node->yes = NULL;
    last_modified_node->no = NULL;

    /* Restore the node back into a leaf with its original answer */
    strncpy(last_modified_node->text, last_modified_old_text, MAX_LEN - 1);
    last_modified_node->text[MAX_LEN - 1] = '\0';
    last_modified_node->is_leaf = 1;

    save_tree(TREE_FILE, root);

    printf("\nLast learning step undone and saved.\n\n");

    undo_available = 0;
    last_modified_node = NULL;
}

/* ---------------------------------------------------------
 * Reset memory
 * --------------------------------------------------------- */

static void reset_memory(void) {
    printf("\nThis will erase everything the AI has learned!\n");
    int confirm = ask_yes_no("Are you sure you want to reset memory");
    if (!confirm) {
        printf("Reset cancelled.\n\n");
        return;
    }

    free_tree(root);
    root = build_default_tree();
    save_tree(TREE_FILE, root);

    stat_correct = 0;
    stat_wrong = 0;
    save_stats();

    undo_available = 0;
    last_modified_node = NULL;

    printf("\nMemory has been reset to default.\n\n");
}

/* ---------------------------------------------------------
 * Game engine
 * --------------------------------------------------------- */

/* Play a single round: walk the tree (iteratively) by asking yes/no
 * questions, then check the final guess. If wrong, trigger learning. */
static void play_round(void) {
    if (root == NULL) {
        printf("Error: knowledge base is empty.\n");
        return;
    }

    printf("\nThink of an object, animal, or thing... and let's begin!\n\n");

    Node *current = root;

    /* Iterative traversal: follow yes/no answers down the tree
     * until a leaf (an answer/guess) is reached. */
    while (!current->is_leaf) {
        int answer = ask_yes_no(current->text);
        current = answer ? current->yes : current->no;
    }

    printf("\nAI: Is it a %s?\n", current->text);
    int correct = ask_yes_no("Am I right");

    if (correct) {
        printf("\nAI: Great, I guessed it correctly!\n\n");
        stat_correct++;
        save_stats();
    } else {
        stat_wrong++;
        save_stats();
        learn_new_object(current);
        save_tree(TREE_FILE, root);
    }
}

/* ---------------------------------------------------------
 * Menu / main
 * --------------------------------------------------------- */

static void print_menu(void) {
    printf("==========================================\n");
    printf(" AI Guessing Game - Self-Learning Decision Tree\n");
    printf("==========================================\n");
    printf("1. Play a round\n");
    printf("2. View statistics\n");
    printf("3. Visualize knowledge tree\n");
    printf("4. Undo last learning\n");
    printf("5. Reset memory\n");
    printf("6. Exit\n");
    printf("==========================================\n");
}

int main(void) {
    /* Load the tree from disk, or start with the default if no
     * saved knowledge base exists yet. */
    root = load_tree(TREE_FILE);
    if (root == NULL) {
        root = build_default_tree();
        save_tree(TREE_FILE, root);
        printf("No existing knowledge found. Starting with a fresh knowledge base.\n");
    } else {
        printf("Knowledge base loaded from '%s'.\n", TREE_FILE);
    }

    load_stats();

    char choice[MAX_LEN];
    int running = 1;

    while (running) {
        printf("\n");
        print_menu();
        printf("Choose an option (1-6): ");
        read_line(choice, sizeof(choice));

        if (strcmp(choice, "1") == 0) {
            play_round();
        } else if (strcmp(choice, "2") == 0) {
            show_stats();
        } else if (strcmp(choice, "3") == 0) {
            printf("\n----- Knowledge Tree -----\n");
            print_tree(root, 0);
            printf("---------------------------\n\n");
        } else if (strcmp(choice, "4") == 0) {
            undo_last_learning();
        } else if (strcmp(choice, "5") == 0) {
            reset_memory();
        } else if (strcmp(choice, "6") == 0) {
            printf("\nSaving and exiting. Goodbye!\n");
            running = 0;
        } else {
            printf("\nInvalid choice. Please enter a number from 1 to 6.\n");
        }
    }

    save_tree(TREE_FILE, root);
    save_stats();
    free_tree(root);

    return 0;
}

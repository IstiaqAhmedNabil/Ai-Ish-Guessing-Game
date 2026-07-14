/*
 * AI Guessing Game — Self-Learning Decision Tree
 * BUBT | CSE | Intake 56 | Section 3 | CSE100
 *
 * Windows-compatible console UI:
 *   - SetConsoleOutputCP(65001)  → UTF-8 so box chars render correctly
 *   - ENABLE_VIRTUAL_TERMINAL_PROCESSING → ANSI colors in cmd/PowerShell
 *   - ASCII fallback box chars if UTF-8 init fails
 */

/* ── Windows / POSIX detection ───────────────────────────────────────────── */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define IS_WINDOWS 1
#else
#define IS_WINDOWS 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tree.h"

/* ── Layout constants ────────────────────────────────────────────────────── */
#define COL 80     /* assumed terminal width                               */
#define BOX_W 60   /* outer box width including border chars               */
#define INNER_W 56 /* usable text width  = BOX_W - 4                      */

/* ── ANSI color codes ────────────────────────────────────────────────────── */
/* Populated at runtime once we know ANSI is supported */
static const char *C_RESET = "";
static const char *C_BOLD = "";
static const char *C_GREEN = "";
static const char *C_BGREEN = "";
static const char *C_CYAN = "";
static const char *C_BCYAN = "";
static const char *C_YELLOW = "";
static const char *C_RED = "";
static const char *C_MAGENTA = "";
static const char *C_WHITE = "";
static const char *C_BLUE = "";
static const char *C_GRAY = "";

static void enable_ansi(void)
{
#if IS_WINDOWS
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h == INVALID_HANDLE_VALUE)
        return;
    if (!GetConsoleMode(h, &mode))
        return;
    /* ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004 */
    if (!SetConsoleMode(h, mode | 0x0004))
        return;
#endif
    /* If we reach here ANSI is available */
    C_RESET = "\033[0m";
    C_BOLD = "\033[1m";
    C_GREEN = "\033[32m";
    C_BGREEN = "\033[1;32m";
    C_CYAN = "\033[36m";
    C_BCYAN = "\033[1;36m";
    C_YELLOW = "\033[1;33m";
    C_RED = "\033[1;31m";
    C_MAGENTA = "\033[1;35m";
    C_WHITE = "\033[1;37m";
    C_BLUE = "\033[1;34m";
    C_GRAY = "\033[90m";
}

/* ── Box-drawing characters (chosen at runtime) ──────────────────────────── */
/* Double-line corners / sides */
static const char *B_TL = "+"; /* ╔  (overridden after UTF-8 init)        */
static const char *B_TR = "+"; /* ╗ */
static const char *B_BL = "+"; /* ╚ */
static const char *B_BR = "+"; /* ╝ */
static const char *B_H = "-";  /* ═ */
static const char *B_V = "|";  /* ║ */
static const char *B_ML = "+"; /* ╠ */
static const char *B_MR = "+"; /* ╣ */
/* Thin-line corners / sides */
static const char *B_TLS = "+"; /* ┌ */
static const char *B_TRS = "+"; /* ┐ */
static const char *B_BLS = "+"; /* └ */
static const char *B_BRS = "+"; /* ┘ */
static const char *B_HS = "-";  /* ─ */
static const char *B_VS = "|";  /* │ */

static void enable_utf8(void)
{
#if IS_WINDOWS
    if (!SetConsoleOutputCP(65001))
        return; /* try to switch to UTF-8 */
#endif
    /* UTF-8 box-drawing strings */
    B_TL = "\xe2\x95\x94";  /* ╔ */
    B_TR = "\xe2\x95\x97";  /* ╗ */
    B_BL = "\xe2\x95\x9a";  /* ╚ */
    B_BR = "\xe2\x95\x9d";  /* ╝ */
    B_H = "\xe2\x95\x90";   /* ═ */
    B_V = "\xe2\x95\x91";   /* ║ */
    B_ML = "\xe2\x95\xa0";  /* ╠ */
    B_MR = "\xe2\x95\xa3";  /* ╣ */
    B_TLS = "\xe2\x94\x8c"; /* ┌ */
    B_TRS = "\xe2\x94\x90"; /* ┐ */
    B_BLS = "\xe2\x94\x94"; /* └ */
    B_BRS = "\xe2\x94\x98"; /* ┘ */
    B_HS = "\xe2\x94\x80";  /* ─ */
    B_VS = "\xe2\x94\x82";  /* │ */
}

/* ── Persistent files ────────────────────────────────────────────────────── */
#define TREE_FILE "tree.dat"
#define STATS_FILE "stats.dat"

/* ── Global state ────────────────────────────────────────────────────────── */
static Node *root = NULL;
static int stat_correct = 0;
static int stat_wrong = 0;
static Node *last_modified_node = NULL;
static char last_modified_old_text[MAX_LEN];
static int undo_available = 0;

/* ═══════════════════════════════════════════════════════════════════════════
 *  LOW-LEVEL DRAWING PRIMITIVES
 * ═══════════════════════════════════════════════════════════════════════════ */

static void spaces(int n)
{
    for (int i = 0; i < n; i++)
        putchar(' ');
}

/* Left-pad so BOX_W content is centered in COL columns; returns pad used. */
static int center_pad(int content_width)
{
    int pad = (COL - content_width) / 2;
    if (pad < 0)
        pad = 0;
    spaces(pad);
    return pad;
}

/* Repeat a string n times (for single-char wide draw chars) */
static void rep(const char *s, int n)
{
    for (int i = 0; i < n; i++)
        fputs(s, stdout);
}

/* ── Double-line box primitives ─────────────────────────────────────────── */

static void box_top(const char *bc)
{ /* ╔════╗  */
    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_TL, stdout);
    rep(B_H, BOX_W - 2);
    fputs(B_TR, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}
static void box_bot(const char *bc)
{ /* ╚════╝  */
    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_BL, stdout);
    rep(B_H, BOX_W - 2);
    fputs(B_BR, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}
static void box_div(const char *bc)
{ /* ╠════╣  */
    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_ML, stdout);
    rep(B_H, BOX_W - 2);
    fputs(B_MR, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}
static void box_empty(const char *bc)
{ /* ║    ║  */
    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_V, stdout);
    fputs(C_RESET, stdout);
    spaces(BOX_W - 2);
    fputs(bc, stdout);
    fputs(B_V, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}

/*
 * ║  <text centered in INNER_W>  ║
 * bc = border color string, tc = text color string
 * text must contain only printable ASCII (no embedded ANSI).
 */
static void box_mid(const char *bc, const char *tc, const char *text)
{
    int tlen = (int)strlen(text);
    int lp = (INNER_W - tlen) / 2;
    int rp = INNER_W - tlen - lp;
    if (lp < 0)
    {
        lp = 0;
        rp = 0;
    }

    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_V, stdout);
    fputs(C_RESET, stdout);
    putchar(' ');
    spaces(lp);
    fputs(tc, stdout);
    fputs(text, stdout);
    fputs(C_RESET, stdout);
    spaces(rp);
    putchar(' ');
    fputs(bc, stdout);
    fputs(B_V, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}

/* ── Thin-line box primitives (for question prompts) ────────────────────── */

static void tbox_top(const char *bc)
{
    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_TLS, stdout);
    rep(B_HS, BOX_W - 2);
    fputs(B_TRS, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}
static void tbox_bot(const char *bc)
{
    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_BLS, stdout);
    rep(B_HS, BOX_W - 2);
    fputs(B_BRS, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}
/* Same centered content row, using thin border char */
static void tbox_mid(const char *bc, const char *tc, const char *text)
{
    int tlen = (int)strlen(text);
    int lp = (INNER_W - tlen) / 2;
    int rp = INNER_W - tlen - lp;
    if (lp < 0)
    {
        lp = 0;
        rp = 0;
    }

    center_pad(BOX_W);
    fputs(bc, stdout);
    fputs(B_VS, stdout);
    fputs(C_RESET, stdout);
    putchar(' ');
    spaces(lp);
    fputs(tc, stdout);
    fputs(text, stdout);
    fputs(C_RESET, stdout);
    spaces(rp);
    putchar(' ');
    fputs(bc, stdout);
    fputs(B_VS, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}

/* ── Miscellaneous ───────────────────────────────────────────────────────── */

/* Clear screen via ANSI escape */
static void cls(void) { fputs("\033[2J\033[H", stdout); }

/* A plain centered line (no box) */
static void cline(const char *color, const char *text)
{
    int len = (int)strlen(text);
    center_pad(len);
    fputs(color, stdout);
    fputs(text, stdout);
    fputs(C_RESET, stdout);
    putchar('\n');
}

/* ── Persistent header drawn on every screen ────────────────────────────── */
static void draw_header(void)
{
    putchar('\n');
    box_top(C_BGREEN);
    box_empty(C_BGREEN);
    box_mid(C_BGREEN, C_YELLOW, "  AI  GUESSING  GAME  ");
    box_mid(C_BGREEN, C_WHITE, "Self-Learning Decision Tree");
    box_empty(C_BGREEN);
    box_div(C_BGREEN);
    box_mid(C_BGREEN, C_GRAY, "BUBT | CSE | Intake 56 | Sec 3 | CSE100");
    box_bot(C_BGREEN);
    putchar('\n');
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  INPUT HELPERS
 * ═══════════════════════════════════════════════════════════════════════════ */

static void read_line(char *buf, int size)
{
    if (fgets(buf, size, stdin) == NULL)
    {
        putchar('\n');
        cline(C_YELLOW, "Input ended. Goodbye!");
        putchar('\n');
        exit(0);
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
    {
        buf[len - 1] = '\0';
        len--;
    }
}

/*
 * Centered yes/no prompt in a thin box.
 * 'question' must be plain ASCII, max ~INNER_W chars.
 * Returns 1 for yes, 0 for no.
 */
static int ask_yes_no(const char *question)
{
    char buf[MAX_LEN];
    while (1)
    {
        tbox_top(C_CYAN);
        tbox_mid(C_CYAN, C_WHITE, question);
        tbox_mid(C_CYAN, C_GRAY, "[ yes / y   |   no / n ]");
        tbox_bot(C_CYAN);

        /* centered prompt arrow */
        int pw = 6;
        center_pad((COL - pw) / 2);
        fputs(C_YELLOW, stdout);
        fputs(" >> ", stdout);
        fputs(C_RESET, stdout);
        read_line(buf, sizeof(buf));

        for (int i = 0; buf[i]; i++)
            buf[i] = (char)tolower((unsigned char)buf[i]);

        if (strcmp(buf, "yes") == 0 || strcmp(buf, "y") == 0)
        {
            putchar('\n');
            return 1;
        }
        if (strcmp(buf, "no") == 0 || strcmp(buf, "n") == 0)
        {
            putchar('\n');
            return 0;
        }

        putchar('\n');
        cline(C_RED, "Please answer:  yes / y   or   no / n");
        putchar('\n');
    }
}

/* Centered free-text prompt; re-prompts on empty input. */
static void ask_text(const char *prompt, char *out, int size)
{
    while (1)
    {
        int pw = (int)strlen(prompt) + 2;
        center_pad((COL - pw) / 2);
        fputs(C_YELLOW, stdout);
        fputs(prompt, stdout);
        fputs(C_RESET, stdout);
        read_line(out, size);
        if (strlen(out) > 0)
            return;
        cline(C_RED, "Please enter a non-empty answer.");
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  STATISTICS
 * ═══════════════════════════════════════════════════════════════════════════ */

static void load_stats(void)
{
    FILE *fp = fopen(STATS_FILE, "r");
    if (!fp)
    {
        stat_correct = stat_wrong = 0;
        return;
    }
    if (fscanf(fp, "%d %d", &stat_correct, &stat_wrong) != 2)
        stat_correct = stat_wrong = 0;
    fclose(fp);
}

static void save_stats(void)
{
    FILE *fp = fopen(STATS_FILE, "w");
    if (!fp)
        return;
    fprintf(fp, "%d %d\n", stat_correct, stat_wrong);
    fclose(fp);
}

static void show_stats(void)
{
    cls();
    draw_header();

    int total = stat_correct + stat_wrong;
    double pct = total > 0 ? (100.0 * stat_correct) / total : 0.0;
    char line[INNER_W + 1];

    box_top(C_BCYAN);
    box_mid(C_BCYAN, C_YELLOW, "STATISTICS");
    box_div(C_BCYAN);
    box_empty(C_BCYAN);

    snprintf(line, sizeof(line), "Correct Guesses  :  %d", stat_correct);
    box_mid(C_BCYAN, C_BGREEN, line);

    snprintf(line, sizeof(line), "Wrong   Guesses  :  %d", stat_wrong);
    box_mid(C_BCYAN, C_RED, line);

    snprintf(line, sizeof(line), "Total   Games    :  %d", total);
    box_mid(C_BCYAN, C_WHITE, line);

    snprintf(line, sizeof(line), "Accuracy         :  %.1f%%", pct);
    box_mid(C_BCYAN, C_YELLOW, line);

    box_empty(C_BCYAN);
    box_bot(C_BCYAN);
    putchar('\n');
    cline(C_GRAY, "Press ENTER to return to menu...");
    char tmp[8];
    read_line(tmp, sizeof(tmp));
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  DEFAULT KNOWLEDGE BASE
 * ═══════════════════════════════════════════════════════════════════════════ */

static Node *build_default_tree(void)
{
    return create_node("Dog", 1);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  LEARNING MODULE
 * ═══════════════════════════════════════════════════════════════════════════ */

static void learn_new_object(Node *wrong_leaf)
{
    char correct_answer[MAX_LEN];
    char new_question[MAX_LEN];
    char old_guess[MAX_LEN];
    char prompt[MAX_LEN];

    putchar('\n');
    box_top(C_MAGENTA);
    box_mid(C_MAGENTA, C_YELLOW, "LEARNING MODE");
    box_div(C_MAGENTA);
    box_mid(C_MAGENTA, C_WHITE, "Help me learn! I will remember this.");
    box_empty(C_MAGENTA);
    box_bot(C_MAGENTA);
    putchar('\n');

    cline(C_WHITE, "What were you thinking of?");
    ask_text(" >> ", correct_answer, sizeof(correct_answer));
    putchar('\n');

    /* Safe truncating prompt build */
    snprintf(prompt, sizeof(prompt),
             "A question to tell '%s' from '%s'?",
             correct_answer, wrong_leaf->text);
    /* Truncate to INNER_W if needed */
    if ((int)strlen(prompt) > INNER_W)
        prompt[INNER_W] = '\0';

    cline(C_WHITE, "Give me a yes/no question:");
    cline(C_GRAY, prompt);
    ask_text(" >> ", new_question, sizeof(new_question));
    putchar('\n');

    snprintf(prompt, sizeof(prompt),
             "For '%s', is the answer YES?", correct_answer);
    if ((int)strlen(prompt) > INNER_W)
        prompt[INNER_W] = '\0';
    int answer_for_new = ask_yes_no(prompt);

    /* Save state for undo */
    strncpy(old_guess, wrong_leaf->text, MAX_LEN - 1);
    strncpy(last_modified_old_text, old_guess, MAX_LEN - 1);
    old_guess[MAX_LEN - 1] = '\0';
    last_modified_old_text[MAX_LEN - 1] = '\0';
    last_modified_node = wrong_leaf;
    undo_available = 1;

    /* Convert leaf to question node */
    strncpy(wrong_leaf->text, new_question, MAX_LEN - 1);
    wrong_leaf->text[MAX_LEN - 1] = '\0';
    wrong_leaf->is_leaf = 0;

    Node *new_obj = create_node(correct_answer, 1);
    Node *old_obj = create_node(old_guess, 1);

    if (answer_for_new)
    {
        wrong_leaf->yes = new_obj;
        wrong_leaf->no = old_obj;
    }
    else
    {
        wrong_leaf->yes = old_obj;
        wrong_leaf->no = new_obj;
    }

    putchar('\n');
    box_top(C_BGREEN);
    box_mid(C_BGREEN, C_YELLOW, "Learned! Thank you.");
    box_mid(C_BGREEN, C_WHITE, "I will remember this next time.");
    box_bot(C_BGREEN);
    putchar('\n');
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  UNDO LAST LEARNING
 * ═══════════════════════════════════════════════════════════════════════════ */

static void undo_last_learning(void)
{
    cls();
    draw_header();

    if (!undo_available || !last_modified_node)
    {
        box_top(C_RED);
        box_mid(C_RED, C_YELLOW, "Nothing to undo.");
        box_bot(C_RED);
    }
    else
    {
        free_tree(last_modified_node->yes);
        free_tree(last_modified_node->no);
        last_modified_node->yes = last_modified_node->no = NULL;

        strncpy(last_modified_node->text, last_modified_old_text, MAX_LEN - 1);
        last_modified_node->text[MAX_LEN - 1] = '\0';
        last_modified_node->is_leaf = 1;

        save_tree(TREE_FILE, root);
        undo_available = 0;
        last_modified_node = NULL;

        box_top(C_BGREEN);
        box_mid(C_BGREEN, C_YELLOW, "Last learning step undone.");
        box_mid(C_BGREEN, C_WHITE, "Knowledge base saved.");
        box_bot(C_BGREEN);
    }
    putchar('\n');
    cline(C_GRAY, "Press ENTER to return to menu...");
    char tmp[8];
    read_line(tmp, sizeof(tmp));
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  RESET MEMORY
 * ═══════════════════════════════════════════════════════════════════════════ */

static void reset_memory(void)
{
    cls();
    draw_header();

    box_top(C_RED);
    box_mid(C_RED, C_YELLOW, "WARNING: RESET MEMORY");
    box_div(C_RED);
    box_mid(C_RED, C_WHITE, "This will erase ALL learned knowledge!");
    box_empty(C_RED);
    box_bot(C_RED);
    putchar('\n');

    if (!ask_yes_no("Are you sure you want to reset?"))
    {
        cline(C_BGREEN, "Reset cancelled. No changes made.");
        putchar('\n');
        cline(C_GRAY, "Press ENTER to return to menu...");
        char tmp[8];
        read_line(tmp, sizeof(tmp));
        return;
    }

    free_tree(root);
    root = build_default_tree();
    save_tree(TREE_FILE, root);
    stat_correct = stat_wrong = 0;
    save_stats();
    undo_available = 0;
    last_modified_node = NULL;

    putchar('\n');
    box_top(C_BGREEN);
    box_mid(C_BGREEN, C_YELLOW, "Memory reset to default.");
    box_mid(C_BGREEN, C_WHITE, "Starting fresh with 'Dog'.");
    box_bot(C_BGREEN);
    putchar('\n');
    cline(C_GRAY, "Press ENTER to return to menu...");
    char tmp[8];
    read_line(tmp, sizeof(tmp));
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  TREE VISUALIZATION
 * ═══════════════════════════════════════════════════════════════════════════ */

static void print_tree_centered(Node *node, int depth)
{
    if (!node)
        return;

    /* Indent: 2 spaces per depth level, then center the whole line */
    int indent = depth * 3;
    /* Approximate line length for centering */
    int textlen = (int)strlen(node->text) + 4 + indent; /* 4 = "[ ]" + space */
    int lpad = (COL - textlen) / 2;
    if (lpad < 2)
        lpad = 2;

    spaces(lpad);
    spaces(indent);

    if (node->is_leaf)
    {
        fputs(C_BGREEN, stdout);
        fputs("[", stdout);
        fputs(C_YELLOW, stdout);
        fputs(node->text, stdout);
        fputs(C_BGREEN, stdout);
        fputs("]", stdout);
        fputs(C_RESET, stdout);
    }
    else
    {
        fputs(C_BCYAN, stdout);
        fputs("(", stdout);
        fputs(C_WHITE, stdout);
        fputs(node->text, stdout);
        fputs(C_BCYAN, stdout);
        fputs(")", stdout);
        fputs(C_RESET, stdout);
    }
    putchar('\n');

    if (!node->is_leaf)
    {
        /* YES branch */
        spaces(lpad + indent + 2);
        fputs(C_BGREEN, stdout);
        fputs("|-- YES: ", stdout);
        fputs(C_RESET, stdout);
        putchar('\n');
        print_tree_centered(node->yes, depth + 1);

        /* NO branch */
        spaces(lpad + indent + 2);
        fputs(C_RED, stdout);
        fputs("|-- NO:  ", stdout);
        fputs(C_RESET, stdout);
        putchar('\n');
        print_tree_centered(node->no, depth + 1);
    }
}

static void show_tree(void)
{
    cls();
    draw_header();

    box_top(C_BCYAN);
    box_mid(C_BCYAN, C_YELLOW, "KNOWLEDGE TREE");
    box_div(C_BCYAN);
    box_mid(C_BCYAN, C_GRAY, "( question )    [ answer/guess ]");
    box_bot(C_BCYAN);
    putchar('\n');

    print_tree_centered(root, 0);

    putchar('\n');
    cline(C_GRAY, "Press ENTER to return to menu...");
    char tmp[8];
    read_line(tmp, sizeof(tmp));
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  GAME ENGINE
 * ═══════════════════════════════════════════════════════════════════════════ */

static void play_round(void)
{
    if (!root)
    {
        cline(C_RED, "Error: knowledge base is empty.");
        return;
    }

    cls();
    draw_header();
    box_top(C_BGREEN);
    box_mid(C_BGREEN, C_YELLOW, "Think of any object, animal, or thing...");
    box_mid(C_BGREEN, C_WHITE, "I will guess it with YES / NO questions!");
    box_empty(C_BGREEN);
    box_mid(C_BGREEN, C_GRAY, "Answer every question honestly.");
    box_bot(C_BGREEN);
    putchar('\n');
    cline(C_GRAY, "Press ENTER when you have something in mind...");
    char tmp[8];
    read_line(tmp, sizeof(tmp));

    Node *cur = root;
    int qno = 0;

    while (!cur->is_leaf)
    {
        qno++;
        cls();
        draw_header();

        char hdr[32];
        snprintf(hdr, sizeof(hdr), "QUESTION  #%d", qno);
        box_top(C_BCYAN);
        box_mid(C_BCYAN, C_YELLOW, hdr);
        box_bot(C_BCYAN);
        putchar('\n');

        /* Truncate long questions to INNER_W */
        char q[MAX_LEN];
        strncpy(q, cur->text, INNER_W);
        q[INNER_W] = '\0';

        int ans = ask_yes_no(q);
        cur = ans ? cur->yes : cur->no;
    }

    /* Final guess */
    cls();
    draw_header();
    char gline[MAX_LEN];
    snprintf(gline, sizeof(gline), "Is it a  %s ?", cur->text);
    if ((int)strlen(gline) > INNER_W)
        gline[INNER_W] = '\0';

    box_top(C_YELLOW);
    box_mid(C_YELLOW, C_YELLOW, "MY GUESS");
    box_div(C_YELLOW);
    box_empty(C_YELLOW);
    box_mid(C_YELLOW, C_WHITE, gline);
    box_empty(C_YELLOW);
    box_bot(C_YELLOW);
    putchar('\n');

    if (ask_yes_no("Am I right?"))
    {
        stat_correct++;
        save_stats();
        putchar('\n');
        box_top(C_BGREEN);
        box_mid(C_BGREEN, C_YELLOW, "I GOT IT!");
        box_mid(C_BGREEN, C_WHITE, "Great, I guessed it correctly!");
        box_bot(C_BGREEN);
    }
    else
    {
        stat_wrong++;
        save_stats();
        putchar('\n');
        box_top(C_RED);
        box_mid(C_RED, C_YELLOW, "I FAILED...");
        box_mid(C_RED, C_WHITE, "Let me learn from this!");
        box_bot(C_RED);
        putchar('\n');
        learn_new_object(cur);
        save_tree(TREE_FILE, root);
    }

    putchar('\n');
    cline(C_GRAY, "Press ENTER to return to menu...");
    char tmp2[8];
    read_line(tmp2, sizeof(tmp2));
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  MAIN MENU
 * ═══════════════════════════════════════════════════════════════════════════ */

static void print_menu(void)
{
    cls();
    draw_header();

    /* Live stats bar */
    int total = stat_correct + stat_wrong;
    double pct = total > 0 ? (100.0 * stat_correct) / total : 0.0;
    char sline[INNER_W + 1];
    snprintf(sline, sizeof(sline),
             "Wins: %d   |   Losses: %d   |   Accuracy: %.0f%%",
             stat_correct, stat_wrong, pct);

    box_top(C_BLUE);
    box_mid(C_BLUE, C_GRAY, sline);
    box_bot(C_BLUE);
    putchar('\n');

    /* Menu */
    box_top(C_BCYAN);
    box_mid(C_BCYAN, C_YELLOW, "MAIN  MENU");
    box_div(C_BCYAN);
    box_empty(C_BCYAN);
    box_mid(C_BCYAN, C_BGREEN, "  1.   Play a Round          ");
    box_mid(C_BCYAN, C_CYAN, "  2.   View Statistics       ");
    box_mid(C_BCYAN, C_CYAN, "  3.   Visualize Knowledge   ");
    box_mid(C_BCYAN, C_YELLOW, "  4.   Undo Last Learning    ");
    box_mid(C_BCYAN, C_RED, "  5.   Reset Memory          ");
    box_mid(C_BCYAN, C_GRAY, "  6.   Exit                  ");
    box_empty(C_BCYAN);
    box_bot(C_BCYAN);
    putchar('\n');

    center_pad((COL - 26) / 2);
    fputs(C_YELLOW, stdout);
    fputs("  Enter choice (1-6) >> ", stdout);
    fputs(C_RESET, stdout);
}

/* ═══════════════════════════════════════════════════════════════════════════
 *  MAIN
 * ═══════════════════════════════════════════════════════════════════════════ */

int main(void)
{
    /* 1. Switch Windows console to UTF-8 (no-op on Linux/macOS) */
    enable_utf8();
    /* 2. Enable ANSI escape codes (needed on Windows 10+) */
    enable_ansi();

    /* 3. Load knowledge base */
    root = load_tree(TREE_FILE);
    if (!root)
    {
        root = build_default_tree();
        save_tree(TREE_FILE, root);
    }
    load_stats();

    char choice[MAX_LEN];
    int running = 1;

    while (running)
    {
        print_menu();
        read_line(choice, sizeof(choice));

        if (strcmp(choice, "1") == 0)
            play_round();
        else if (strcmp(choice, "2") == 0)
            show_stats();
        else if (strcmp(choice, "3") == 0)
            show_tree();
        else if (strcmp(choice, "4") == 0)
            undo_last_learning();
        else if (strcmp(choice, "5") == 0)
            reset_memory();
        else if (strcmp(choice, "6") == 0)
        {
            cls();
            draw_header();
            box_top(C_BGREEN);
            box_mid(C_BGREEN, C_YELLOW, "Thanks for playing!");
            box_mid(C_BGREEN, C_WHITE, "Knowledge saved. See you next time!");
            box_bot(C_BGREEN);
            putchar('\n');
            running = 0;
        }
        else
        {
            putchar('\n');
            cline(C_RED, "Invalid choice. Please enter 1 to 6.");
            putchar('\n');
        }
    }

    save_tree(TREE_FILE, root);
    save_stats();
    free_tree(root);
    return 0;
}
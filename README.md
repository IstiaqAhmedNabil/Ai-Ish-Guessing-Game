<div align="center">

# 🤖 AI Guessing Game

### Self-Learning Decision Tree — Console Application

![Language](https://img.shields.io/badge/Language-C%20%28C99%29-blue?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey?style=flat-square)
![Type](https://img.shields.io/badge/Type-Console--Based-green?style=flat-square)
![Status](https://img.shields.io/badge/Status-Complete-brightgreen?style=flat-square)

_A console-based artificial intelligence that plays a "20 Questions" style guessing game,_
_teaches itself from every mistake, and permanently grows its knowledge base across sessions._

---

**Bangladesh University of Business & Technology (BUBT)**
Department of Computer Science & Engineering
Intake 56 · Section 3 · 3rd Semester

</div>

---

## 📋 Table of Contents

1. [Project Overview](#1-project-overview)
2. [System Architecture](#2-system-architecture)
3. [Project Structure](#3-project-structure)
4. [Data Structure Design](#4-data-structure-design)
5. [Core Algorithms & Flow](#5-core-algorithms--flow)
6. [File Persistence](#6-file-persistence)
7. [Building & Running](#7-building--running)
8. [Feature Reference](#8-feature-reference)
9. [Example Session](#9-example-session)
10. [Design Decisions & Limitations](#10-design-decisions--limitations)

---

## 1. Project Overview

The **AI Guessing Game** is a self-improving expert system written in C. It narrows down any object, animal, or concept the user is thinking of through a binary sequence of yes/no questions. Its defining characteristic is its **learning mechanism** — every incorrect guess causes the AI to restructure its internal knowledge base, making it smarter with each session.

### Core Properties

| Property                | Detail                                                          |
| ----------------------- | --------------------------------------------------------------- |
| **Language**            | C (C99)                                                         |
| **Core Data Structure** | Dynamic binary decision tree                                    |
| **Persistence**         | Plain-text pre-order serialization (`tree.dat`, `stats.dat`)    |
| **UI Style**            | Centered, ANSI-colored console interface with UTF-8 box drawing |
| **Platform**            | Windows (cmd/PowerShell) · Linux · macOS                        |
| **Memory Model**        | Fully heap-allocated via `malloc` / `free` — zero leaks         |

---

## 2. System Architecture

The codebase is separated into two clear layers: a reusable **Tree ADT** (`tree.h` / `tree.c`) and the **Game Application** (`main.c`) built on top of it.

```
┌─────────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER  (main.c)                  │
│                                                                 │
│   ┌─────────────┐  ┌─────────────┐  ┌──────────────────────┐   │
│   │ Menu & Input│  │ Game Engine │  │   Learning Module    │   │
│   │   Engine    │  │             │  │                      │   │
│   └──────┬──────┘  └──────┬──────┘  └──────────┬───────────┘   │
│          │                │                    │               │
│   ┌──────┴──────┐  ┌──────┴──────────────────┐ │               │
│   │  Statistics │  │    Undo / Reset Module  │ │               │
│   │   Module   │  │                         │ │               │
│   └──────┬──────┘  └─────────────────────────┘ │               │
│          │                                     │               │
└──────────┼─────────────────────────────────────┼───────────────┘
           │                                     │
           ▼                                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                     TREE ADT  (tree.h / tree.c)                 │
│                                                                 │
│    create_node()   free_tree()   save_tree()   load_tree()      │
│                       print_tree()                              │
└──────────────────────────────┬──────────────────────────────────┘
                               │  File I/O
                    ┌──────────┴──────────┐
                    │                     │
               ┌────┴─────┐        ┌──────┴─────┐
               │ tree.dat │        │ stats.dat  │
               └──────────┘        └────────────┘
```

### Component Responsibilities

| Component           | File              | Responsibility                                                         |
| ------------------- | ----------------- | ---------------------------------------------------------------------- |
| Menu & Input Engine | `main.c`          | Main loop, centered UI rendering, validated yes/no and free-text input |
| Game Engine         | `main.c`          | Iterative tree traversal, round management, guess presentation         |
| Learning Module     | `main.c`          | In-place leaf-to-question conversion, user-guided tree expansion       |
| Statistics Module   | `main.c`          | Win/loss tracking, accuracy calculation, `stats.dat` I/O               |
| Undo / Reset Module | `main.c`          | Single-step undo, confirmation-protected full memory wipe              |
| Tree ADT            | `tree.h / tree.c` | Node lifecycle (alloc/free), pre-order save/load, ASCII visualization  |

---

## 3. Project Structure

```
AI-Guessing-Game/
│
├── main.c          ←  Game engine · UI · learning · stats · undo · reset
├── tree.c          ←  Tree ADT: create · free · save · load · print
├── tree.h          ←  Node struct definition & function prototypes
│
├── tree.dat        ←  Serialized knowledge base   (auto-created at runtime)
├── stats.dat       ←  Win / loss counters          (auto-created at runtime)
│
└── README.md
```

> `tree.dat` and `stats.dat` are generated automatically on first run and do not need to be created manually.

---

## 4. Data Structure Design

### The Node Struct

Every piece of the AI's knowledge is stored as a `Node` allocated on the heap:

```c
#define MAX_LEN 256

typedef struct Node {
    char         text[MAX_LEN];  // question text  OR  object/answer name
    int          is_leaf;        // 0 = question node  |  1 = answer leaf
    struct Node *yes;            // child followed when user answers YES
    struct Node *no;             // child followed when user answers NO
} Node;
```

### Two Types of Nodes

```
  QUESTION NODE  (is_leaf = 0)          ANSWER / LEAF NODE  (is_leaf = 1)
  ┌─────────────────────────────┐        ┌────────────────────────────────┐
  │  text:  "Is it an animal?" │        │  text:  "Dog"                  │
  │  yes  ──────────► child    │        │  yes  =  NULL                  │
  │  no   ──────────► child    │        │  no   =  NULL                  │
  └─────────────────────────────┘        └────────────────────────────────┘
```

### Example Knowledge Tree

The diagram below shows the tree after the AI has learned four objects:

```
                    ┌──────────────────────────┐
                    │    Is it an animal?      │  ← question node
                    └────────────┬─────────────┘
                                 │
                   YES ──────────┘──────────── NO
                   │                            │
       ┌───────────────────────┐    ┌───────────────────────┐
       │   Does it say meow?  │    │  Does it have wheels? │
       └───────────┬───────────┘    └───────────┬───────────┘
                   │                            │
          YES ─────┘─────NO           YES ──────┘──────NO
          │               │           │                 │
       ┌──────┐        ┌──────┐   ┌──────┐         ┌──────────┐
       │ Cat  │        │ Dog  │   │ Car  │         │ Airplane │
       └──────┘        └──────┘   └──────┘         └──────────┘
       (leaf)          (leaf)     (leaf)            (leaf)
```

### Why a Binary Tree?

- Every question has exactly **two outcomes** (yes / no) — a binary tree is a natural, lossless fit.
- **Dynamic growth**: a leaf converts into a question node in O(1) without restructuring any other part of the tree.
- **Pre-order traversal** serializes and reconstructs the full structure unambiguously using a simple line-by-line reader.
- **O(d) lookup per round** where _d_ is the depth of the correct answer's path from root.

---

## 5. Core Algorithms & Flow

### 5.1 Overall Game Flow

```
  START
    │
    ▼
  Load tree from tree.dat
  (or build default single-leaf tree if file absent)
    │
    ▼
  ┌─────────────────────────────────────────────────────┐
  │                   MAIN MENU LOOP                    │
  │                                                     │
  │   1 ─► Play Round      4 ─► Undo Last Learning     │
  │   2 ─► View Stats      5 ─► Reset Memory           │
  │   3 ─► View Tree       6 ─► Exit                   │
  └────────────────────────┬────────────────────────────┘
                           │  user selects 1
                           ▼
                  ┌─────────────────┐
                  │  current = root │
                  └────────┬────────┘
                           │
                    ┌──────▼──────┐
                    │  is_leaf?   │
                    └──────┬──────┘
                     NO    │    YES
              ┌────────────┘    └──────────────────────┐
              ▼                                        ▼
    ┌──────────────────┐                   ┌───────────────────────┐
    │  Ask question    │                   │  AI announces guess   │
    │  current->text   │                   └───────────┬───────────┘
    └────────┬─────────┘                               │
        YES  │  NO                             ┌───────▼────────┐
        ┌────┘  └────┐                         │  Correct?      │
        ▼            ▼                         └───────┬────────┘
  current=yes   current=no                      YES    │    NO
        │            │                    ┌───────┘    └──────────┐
        └────────────┘                    ▼                       ▼
             (loop)                  stat_correct++        stat_wrong++
                                     Save stats            LEARNING MODE
                                     End round             Save tree
                                                           End round
```

---

### 5.2 Learning Algorithm (In-Place Leaf Conversion)

When the AI guesses wrong, the incorrect leaf node is **restructured in-place** — no parent pointer updates needed.

```
  WRONG GUESS DETECTED
         │
         ▼
  Ask:  "What were you thinking of?"
         │  → correct_answer = "Cat"
         ▼
  Ask:  "Give a yes/no question distinguishing
          'Cat' from 'Dog'?"
         │  → new_question = "Does it say meow?"
         ▼
  Ask:  "For 'Cat', is the answer YES?"
         │  → answer_for_new = YES
         ▼
  Save old_guess = "Dog"
  Store node pointer for undo
         │
         ▼
  ┌─────────────────────────────────────────────────────────┐
  │  BEFORE                     AFTER                       │
  │                                                         │
  │   ┌───────┐         ┌──────────────────────────┐        │
  │   │  Dog  │   ──►   │  Does it say meow?       │        │
  │   │ leaf  │         └──────────────┬───────────┘        │
  │   └───────┘                   YES  │  NO                │
  │                          ┌─────────┘  └──────┐          │
  │                          ▼                   ▼          │
  │                       ┌─────┐            ┌──────┐       │
  │                       │ Cat │            │ Dog  │       │
  │                       └─────┘            └──────┘       │
  └─────────────────────────────────────────────────────────┘
         │
         ▼
  save_tree(TREE_FILE, root)
  Learning complete
```

---

### 5.3 Tree Serialization (Pre-Order)

The tree is persisted using **pre-order traversal** (root → YES subtree → NO subtree). Each node becomes exactly one line in `tree.dat`.

```
  FILE FORMAT                   MEANING
  ─────────────────────────     ──────────────────────────────────────
  Q:Does it say meow?       ←   question node; read YES then NO child
  A:Cat                     ←   leaf node (answer); no children follow
  A:Dog                     ←   leaf node (answer); no children follow
  #                         ←   NULL pointer (absent child)
```

**Why pre-order?**
The root is written before its children, so when reading the file top-down, the reader always knows the current node's type before needing to recurse — no lookahead or index required.

```c
// Save  — recursive pre-order write
void save_node(FILE *fp, Node *node) {
    if (!node)           { fprintf(fp, "#\n");          return; }
    if (node->is_leaf)   { fprintf(fp, "A:%s\n", node->text); return; }
    fprintf(fp, "Q:%s\n", node->text);
    save_node(fp, node->yes);   // YES subtree first
    save_node(fp, node->no);    // NO subtree second
}

// Load  — recursive pre-order read
Node* load_node(FILE *fp) {
    fgets(line, MAX_LEN, fp);
    if ("#")    return NULL;                     // empty child
    if ("A:…")  return create_node(text, 1);     // leaf
    Node *n = create_node(text, 0);              // question
    n->yes  = load_node(fp);                     // read YES subtree
    n->no   = load_node(fp);                     // read NO subtree
    return n;
}
```

---

## 6. File Persistence

| File        | Format                                        | Created When                    | Updated When                  |
| ----------- | --------------------------------------------- | ------------------------------- | ----------------------------- |
| `tree.dat`  | Pre-order text: `Q:…` / `A:…` / `#`           | No prior file exists on startup | After every learning session  |
| `stats.dat` | Two space-separated integers: `correct wrong` | No prior file exists on startup | After every round and on exit |

**Error Handling:**

```
fopen() returns NULL on read  →  Start fresh (default tree / zero stats)
fopen() returns NULL on write →  Print warning; program continues safely
Unexpected EOF during load    →  Return NULL (treated as empty subtree)
```

---

## 7. Building & Running

### Prerequisites

| Tool          | Requirement                                                            |
| ------------- | ---------------------------------------------------------------------- |
| Compiler      | GCC 7.0+ or any C99-compliant C compiler                               |
| Windows       | cmd or PowerShell on Windows 10 version 1511+ (for ANSI color support) |
| Linux / macOS | Any modern terminal emulator                                           |

### Compile

```bash
gcc -Wall -o ai_guess main.c tree.c
```

### Run

```bash
# Linux / macOS
./ai_guess

# Windows
ai_guess.exe
```

> On Windows, the program automatically calls `SetConsoleOutputCP(65001)` to enable UTF-8 and activates `ENABLE_VIRTUAL_TERMINAL_PROCESSING` for ANSI colors — no manual configuration needed.

---

## 8. Feature Reference

### Required Features

| Feature                   | Where Implemented                                                  |
| ------------------------- | ------------------------------------------------------------------ |
| Binary decision tree      | `Node` struct with `*yes` / `*no` pointers — `tree.c`              |
| File save & load          | `save_tree()` / `load_tree()` — pre-order text serialization       |
| Iterative traversal       | `while (!current->is_leaf)` loop in `play_round()`                 |
| Dynamic memory allocation | `malloc` in `create_node()` · `free` in `free_tree()` (post-order) |
| Self-learning mechanism   | `learn_new_object()` — in-place leaf → question node conversion    |

### Additional Features

| Feature                 | Description                                                                      |
| ----------------------- | -------------------------------------------------------------------------------- |
| **Statistics tracking** | Correct / wrong counts and accuracy %, persisted to `stats.dat`                  |
| **Tree visualization**  | Labeled ASCII tree printed to console with YES / NO branch indicators            |
| **Undo last learning**  | Restores the most recently modified node to its original leaf state and re-saves |
| **Reset memory**        | Confirmation-protected wipe — returns to default single-leaf tree, clears stats  |
| **Centered console UI** | All screens rendered in a centered 60-column box on an 80-column terminal        |
| **ANSI color coding**   | Color-differentiated prompts, results, and borders; Windows-compatible           |

---

## 9. Example Session

### Round 1 — AI Fails, Enters Learning Mode

```
  ╔══════════════════════════════════════════════════╗
  ║                    MY GUESS                      ║
  ╠══════════════════════════════════════════════════╣
  ║                Is it a  Dog ?                    ║
  ╚══════════════════════════════════════════════════╝

  Am I right? >> no

  ╔══════════════════════════════════════════════════╗
  ║                LEARNING MODE                     ║
  ╠══════════════════════════════════════════════════╣
  ║       Help me learn! I will remember this.       ║
  ╚══════════════════════════════════════════════════╝

  What were you thinking of?
   >> cat

  Give me a yes/no question:
   >> Does it say meow?

  For 'cat', is the answer YES?  [ yes / y  |  no / n ]
   >> yes

  ╔══════════════════════════════════════════════════╗
  ║               Learned! Thank you.                ║
  ║         I will remember this next time.          ║
  ╚══════════════════════════════════════════════════╝
```

### Round 2 — AI Uses the Knowledge It Learned

```
  ┌──────────────────────────────────────────────────┐
  │             Does it say meow?                    │
  │          [ yes / y   |   no / n ]                │
  └──────────────────────────────────────────────────┘
   >> yes

  ╔══════════════════════════════════════════════════╗
  ║                    MY GUESS                      ║
  ╠══════════════════════════════════════════════════╣
  ║                Is it a  Cat ?                    ║
  ╚══════════════════════════════════════════════════╝

  Am I right? >> yes

  ╔══════════════════════════════════════════════════╗
  ║                   I GOT IT!                      ║
  ║          Great, I guessed it correctly!          ║
  ╚══════════════════════════════════════════════════╝
```

---

## 10. Design Decisions & Limitations

### Design Decisions

| Decision                                       | Rationale                                                                                                                 |
| ---------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| **Iterative traversal** for game rounds        | Avoids call-stack overflow on deep trees; easier to trace in a debugger                                                   |
| **Recursive traversal** for save / load / free | Pre-order and post-order are most naturally expressed recursively; tree depth is bounded by the number of learned objects |
| **In-place leaf conversion** during learning   | No parent pointer updates required; O(1) structural change                                                                |
| **Pre-order serialization**                    | Root before children — reader reconstructs structure top-down with no lookahead                                           |
| **Fixed `char text[MAX_LEN]`** inside struct   | String lives in the node's own malloc block; eliminates secondary allocation and dangling-pointer risk                    |
| **Explicit `is_leaf` flag**                    | More readable than `(yes == NULL)`; future-proof if single-child nodes are added                                          |
| **Separate `tree.c` / `tree.h`**               | Isolates the reusable Tree ADT from game logic; demonstrates header guards and multi-file compilation                     |

### Known Limitations

| Limitation                      | Notes                                                                                         |
| ------------------------------- | --------------------------------------------------------------------------------------------- |
| No duplicate detection          | The same object or question text can appear in multiple nodes                                 |
| Single undo step only           | Only the most recent learning session can be reverted                                         |
| No automatic balancing          | Degenerate input (always answering YES) creates a linear chain of depth _k−1_ for _k_ objects |
| No concurrent access protection | Last-write-wins if two instances run simultaneously; no file locking                          |
| Fixed 80-column layout          | The centered UI assumes a standard 80-column terminal                                         |

---

<div align="center">

_Bangladesh University of Business & Technology (BUBT)_
_Department of Computer Science & Engineering_
_Intake 56 · Section 3 · 3rd Semester_

</div>

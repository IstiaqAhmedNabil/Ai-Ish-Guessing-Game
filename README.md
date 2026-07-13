# AI Guessing Game with Self-Learning Decision Tree

A console-based "20 Questions" style game written in C. The AI tries to
guess what object/animal/thing you're thinking of by asking a series of
yes/no questions, navigating a binary decision tree. If it guesses wrong,
it **learns** from you and permanently expands its knowledge base, saved
to disk between sessions.

## Files

```
AI-Guessing-Game/
├── main.c       - Game engine, learning module, stats, menu, undo, reset
├── tree.c       - Decision tree implementation (create/free/save/load/print)
├── tree.h       - Node structure & function declarations
├── tree.dat     - Saved decision tree (auto-created on first run)
├── stats.dat    - Saved win/loss statistics (auto-created)
└── README.md
```

## Building

```bash
gcc -Wall -o ai_guess main.c tree.c
```

## Running

```bash
./ai_guess
```

## How it works

### Decision Tree

Each `Node` is either:

- A **question node**: has `text` = a yes/no question, plus `yes` and `no`
  child pointers.
- A **leaf node** (`is_leaf == 1`): `text` = the AI's guess (an object
  name), with no children.

### Game Flow

1. Start at the root node.
2. While the current node is a question, ask it and follow the `yes` or
   `no` branch based on the user's answer (iterative traversal).
3. When a leaf is reached, the AI announces its guess.
4. If correct -> round ends, stats updated.
5. If incorrect -> **learning mode**:
   - Ask what the user was actually thinking of.
   - Ask for a yes/no question that distinguishes the new object from the
     AI's wrong guess.
   - Ask what the answer to that question would be for the new object.
   - The wrong-guess leaf is converted into a new question node with two
     leaf children: the old guess and the new object, placed on the
     correct branches.
   - The updated tree is saved to `tree.dat`.

### Persistence

- `tree.dat` stores the tree using a simple pre-order text serialization
  (`Q:...` for questions, `A:...` for answers, `#` for empty children).
- `stats.dat` stores two integers: number of correct guesses and number
  of wrong guesses.

### Extra Features

- **Statistics**: tracks and displays correct vs. wrong guesses and
  overall accuracy.
- **Tree visualization**: prints the current knowledge tree to the
  console in an indented, labeled format.
- **Undo last learning**: reverts the most recent learning step (turns
  the newly-created question node back into the original leaf and
  removes its new children), then re-saves the tree.
- **Reset memory**: wipes the learned tree and statistics back to the
  default starting state (a single leaf guess: "Dog"), after
  confirmation.

## Example Interaction

```
AI: Is it a Dog?  (the very first guess, before any learning)
Am I right? (yes/no): no

AI: I failed! What were you thinking of?
You: cat

AI: Give me a yes/no question that distinguishes a 'cat' from a 'Dog':
You: Does it say meow?

AI: For a 'cat', what is the answer to that question?
So (yes/no): yes

AI: Learned! Thank you.
```

Next time, the AI will ask "Does it say meow?" before guessing Dog or Cat.

## Notes

- Only one learning step can be undone at a time (the most recent one);
  the undo state is cleared once you start a new round's learning or use
  undo.
- The "undo" / "reset" actions immediately re-save the tree and stats
  files so the on-disk knowledge base stays in sync.

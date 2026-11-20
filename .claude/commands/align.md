---
description: Run the column-alignment format pass from .claude/rules/coding-style.md
argument-hint: "[files or directories; default: blocks changed in this task]"
---

Run the align pass described in section 1 of `.claude/rules/coding-style.md`, exactly as written there.

Scope: $ARGUMENTS. If empty, only the blocks containing lines changed in this task (`git diff` against the last commit
before the task). Never touch `third_party/`.

Whitespace only. Afterwards rebuild, run `ctest --preset default`, and show `git diff --stat`. Commit only if the user
asks, alone, as `style: align columns`. If tests or review for the task are not done yet, say what is still unverified.

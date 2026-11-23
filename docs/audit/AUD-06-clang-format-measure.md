---
id: AUD-06-clang-format-measure
type: audit
size: M
status: active
owner: claude
created: 2026-11-23
supersedes: []
links: [FMT-01-install-config-measure]
---
# clang-format alignment-only config: version and dry-run diff size

## Context

Planning `clang-format` phase 1 (S1 + E2 + O1) needs a `.clang-format` that changes only
alignment. This audit records the tool version and what the config would change today.

## Decision

- Tool: `clang-format 21.1.8` (Ubuntu 26.04 package `clang-format 1:21.1.6-71` family; `clang-format --version`
  prints `Ubuntu clang-format version 21.1.8 (6ubuntu1)`). Output changes between major versions, so the
  version is pinned in `CLAUDE.md` Commands.
- `.clang-format`: LLVM base, `IndentWidth: 4`, `ColumnLimit: 100`, the three alignment options, plus the
  layout options that make the current code (pointer left, short `if`/loops/functions on one line,
  two spaces before trailing comments, bracket alignment) a fixed point as far as possible.
- Measured with `clang-format FILE | diff FILE -` per `.cpp`/`.hpp` (tests/fixtures and `third_party/` excluded),
  tree unchanged (dry run only):

| Directory | Files | Files with a diff | Changed lines | Of which not whitespace-only (`diff -w`) |
|---|---|---|---|---|
| include/ | 17 | 13 | 166 | 30 |
| src/ | 10 | 10 | 170 | 82 |
| tests/ | 20 | 16 | 702 | 240 |

- The whitespace-only part is the expected realignment (O1: outlier blocks get aligned, the 20-column
  exception is dropped). The non-whitespace part is line re-wrapping of calls and conditions that clang-format
  cannot be told to keep (no "keep my wrapping" for long lines). FMT-02 must review it with `git diff` and
  decide per file; wrapping is not alignment, so FMT-02 may need `ColumnLimit` tweaks or accept it.
- `AlignAfterOpenBracket: AlwaysBreak` was tried: about the same non-whitespace count (8/84/252).

## Status

Active. Input to FMT-02.

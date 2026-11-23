# Coding style: layout and comments

Adapted for PyGomo (C++23) from SKILLS_TREE `rules/coding-standards/items/coding-style.md`. Naming and code structure: match
the surrounding code (types `PascalCase`, functions `camelCase`, 4-space indent). Line limit: 100 columns.
Do not reformat code you are not changing; format only the files you touch.

## 1. Layout: clang-format owns it

`.clang-format` (pinned clang-format 21.1.8, see `docs/audit/AUD-07-clang-format-full-style.md`) decides alignment, wrapping,
spacing and include order. Do not align by hand and do not argue with the formatter.

- Write code, then run `clang-format -i` on the files you changed (never `third_party/`, `tests/fixtures`).
- `scripts/check-format.sh` (ctest entry `style_format`) fails on any file the formatter would change. Do not weaken it,
  and do not add `// clang-format off` to make it pass without a reason in a comment.
- Format a file in its own commit (`style: clang-format <area>`) when the change would bury a logic diff.
- The old `/align` pass is retired: there is no hand alignment left to maintain.

## 2. Comments

Principle: comment the why, not the what. Brief; one line if it fits. The reader is a contributor who needs the core idea
of a type or function without reading its body.

- Every public type, function and method in `include/` gets a `///` doc comment directly above the declaration.
  Struct and enum members get a trailing `///<` (or a `///` line above when it does not fit).
- First sentence: a summary in the third person ("Runs one search and waits for its terminal event."), ending with a period.
- Then only what the signature does not say: units, ranges, side effects, error cases (which `EngineError`), ownership and
  lifetime, thread-safety, blocking behaviour.
- Do not restate the code (`++i; // increment i`) or the types already in the signature.
- A file whose role is not obvious from its name starts with a short `//` comment saying what it is for and which layer it
  belongs to (core, adapter, transport).
- Private helpers and code in `src/`: a one-line `//` comment, or none if the name says it all.
- Non-obvious code (workaround, invariant, protocol quirk, ordering or locking constraint): a short `//` comment above it,
  with the protocol spec section, ADR or issue id when there is one.
- No commented-out code; git remembers.
- TODOs: `TODO(owner): action`, or `TODO(<todo code>): action` with a harness todo code. No bare TODO.
- Stay inside 100 columns. Update or delete a comment when the code beside it changes.
- Tests: the `TEST_CASE` name says the behaviour; add a comment only for a non-obvious setup or expectation.

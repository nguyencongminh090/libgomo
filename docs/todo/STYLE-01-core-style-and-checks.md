---
id: STYLE-01-core-style-and-checks
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: code-style-normalization
---
# Code style for core, plus the line-length and doc-comment check scripts

## Scope
- Add `scripts/check-line-length.sh [dir...]`: lists every line over 100 columns in the given dirs (default `include src tests`), exits non-zero if any.
- Add `scripts/check-doc-comments.sh [dir...]`: heuristic (grep/awk); lists public declarations in headers under the given dirs (default `include`) with no `///` line above, exits non-zero if any.
- Apply `.claude/rules/coding-style.md` to `include/pygomo/core`, `src/core`, `tests/core`:
  commit 1 = doc comments + lines within 100 columns; commit 2 = `style: align columns` (whitespace only).

## Don't
- No logic, rename, reorder or API change. Nothing outside the three directories and the two scripts. Never touch `third_party/`.
- Do not wire the scripts into ctest yet (STYLE-04).

## Traps
- The doc-comment heuristic will meet templates, `using` aliases, `operator`s and `[[nodiscard]]` on its own line; fix false positives in the script, never by adding empty comments.
- Wrapping a long line is not whitespace-only: it belongs in commit 1, not the align commit.

## Done-when
Acceptance in `docs/features.json` (`"todo": "STYLE-01"`).

## Evidence
Commits `86d6fd9` (scripts, doc comments, wrapped lines) and `6fa582c` (`style: align columns`).
- scripts: on `include/pygomo/core src/core tests/core` both exit 0. Negative case in the scratchpad: a 101-column `long.cpp` -> `long.cpp:1:...`, exit 1; a header with an undocumented `struct Undocumented` -> `bad.hpp:3: struct Undocumented {`, exit 1. Boundary: a 100-character line with a 2-byte UTF-8 character (101 bytes) passes. On the whole `include/` the doc check flags only real gaps (overrides, constructors, option structs) for STYLE-02/03; false positive fixed in the script: members of a class nested in a private section (`DeadlineEngine::Relay`). Line count on the whole tree before: 90, matching the planning baseline.
- lines, docs: both scripts OK on the STYLE-01 dirs after each commit.
- align: `git diff -w 86d6fd9 6fa582c -- include src tests` prints nothing; numstat equal (no blank lines added or removed). InfoEvent `elapsed`/`principalVariation` stay unaligned (would pass 100 columns).
- green: build, `ctest --preset default` 62/62 (4 live skipped), `check-core-deps.sh` OK after both commits.

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

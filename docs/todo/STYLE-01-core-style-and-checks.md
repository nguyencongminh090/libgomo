---
id: STYLE-01-core-style-and-checks
type: todo
size: M
status: backlog
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

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

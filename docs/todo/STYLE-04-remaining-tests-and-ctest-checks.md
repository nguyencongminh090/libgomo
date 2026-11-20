---
id: STYLE-04-remaining-tests-and-ctest-checks
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: code-style-normalization
---
# Code style for the remaining tests, and the style checks in ctest

## Scope
- Apply `.claude/rules/coding-style.md` to `tests/live`, `tests/smoke`, `tests/main.cpp`:
  commit 1 = comments + lines within 100 columns; commit 2 = `style: align columns` (whitespace only).
- Register `scripts/check-line-length.sh` (whole `include src tests`) and `scripts/check-doc-comments.sh` (whole `include`) as ctest entries, so `ctest --preset default` fails on a regression.
- Do after STYLE-03.

## Don't
- No logic, rename, reorder or API change. Do not touch `third_party/` or `tests/fixtures/`.
- Do not weaken a check to make it pass; fix the code or a real false positive in the script.

## Traps
- `scripts/check-ctest-coverage.sh` checks that ctest covers every doctest case; the new script entries must not break it.
- Live tests are skipped without the engine binaries; their style still counts.

## Done-when
Acceptance in `docs/features.json` (`"todo": "STYLE-04"`).

## Evidence

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

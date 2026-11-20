---
id: STYLE-04-remaining-tests-and-ctest-checks
type: todo
size: M
status: done
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
Commits: comments + wraps (live, smoke, main), `style checks as ctest entries`, and `style: align columns (STYLE-04)` (only the `Terminal` members; `git diff -w HEAD~1 HEAD -- include src tests` prints nothing).
- lines: `check-line-length.sh tests/live tests/smoke tests/main.cpp` exit 0 (before: 2 long lines). The script now accepts files and exits 2 on a grep error (it used to read an error as clean).
- ctest: `style_line_length` (include src tests) and `style_doc_comments` (include) pass. Negative case: a 110-column `tests/zz_long.cpp` made `style_line_length` fail; file removed.
- green: `ctest --preset default` and `ctest --preset tsan` 64/64 (4 live skipped), `check-core-deps.sh` and `check-ctest-coverage.sh` OK.

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

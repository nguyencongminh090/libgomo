---
id: code-style-normalization
type: planning
size: L
status: approved
owner: claude
created: 2026-11-20
supersedes: []
links: []
---
# Project-wide code-style normalization
Discussion for an L-size feature. Open questions must be empty to approve. After approval it is frozen; split todos (each at most M) carry `planning: code-style-normalization`.

## Goal
Bring all project code in `include/`, `src/` and `tests/` to `.claude/rules/coding-style.md`. Columns are aligned so related lines
read as a table. Every public type and function has a doc comment that tells a contributor the core idea (role, errors, ownership,
thread-safety, blocking) without reading the body.

## Non-goals
- No logic, rename, reorder or API change. No new tests beyond style checks. Behaviour is identical before and after.
- `third_party/` (doctest) is never touched. Scripts (`scripts/`, `.mjs`, `.sh`) and docs are out of scope.
- No naming convention change (types `PascalCase`, functions `camelCase` stay).

## Constraints
- Baseline (2026-11-20): 47 files, 4860 lines in `include/`, `src/`, `tests/`; 149 `///` lines already in `include/`;
  90 lines over 100 columns in 27 files (11 in `tests/gomocup/gomocup_engine_test.cpp`).
- `clang-format` is not installed and the repo has no `.clang-format`. Nothing enforces or strips alignment today.
- Build uses warnings-as-errors; every todo must leave `ctest --preset default` and `scripts/check-core-deps.sh` green.
- Whitespace-only changes and comment changes must stay reviewable: alignment in its own commit, separate from comments.
- Edits inside an aligned block later are written unaligned (rule section 1); alignment is re-applied only on `/align`.

## Options
Tooling:
- T1. No formatter. Agent applies the rule by hand per module; checks are scripts. No toolchain change.
- T2. Add `.clang-format` (4-space, 100 columns, `AlignConsecutive*`, `AlignTrailingComments`) and a format check in the build.
  Reproducible, but requires installing clang-format, reformats every line (not only alignment), and its alignment rules
  differ from ours (it cannot skip blocks with a >20-character outlier).
- T3. T1 now; T2 later as its own planning if drift shows up.

Checks (proving each todo):
- C1. Alignment commit is whitespace-only: `git diff -w <before>..<after> -- include src tests` prints nothing.
- C2. Line limit: a script `scripts/check-line-length.sh` fails on lines over 100 columns in `include/ src/ tests/`.
- C3. Doc comments: a script `scripts/check-doc-comments.sh` fails when a public declaration in `include/` (class, struct, enum,
  free function, public method) has no `///` line above it. Heuristic (grep/awk), not a parser.
- C4. Review only (reviewer agent reads the comments). No script.

Split (each todo at most M; two commits each: comments + line length first, then `style: align columns`):
1. STYLE-01 `core`: `include/pygomo/core`, `src/core`, `tests/core` (+ adds the check scripts chosen above).
2. STYLE-02 `parse`, `transport`, `process`: headers, sources, tests.
3. STYLE-03 `engine`, `gomocup`, `gtp`, `katagomo`: headers, sources, tests.
4. STYLE-04 rest of `tests/` (`live`, `smoke`, `main.cpp`, fixtures if C++) and turning the check scripts into ctest entries.

## Decision
Approved by owner 2026-11-20: T3 + C1 + C2 + C3, split STYLE-01..04 as above.
Feature acceptance per todo: `ctest --preset default`, `scripts/check-core-deps.sh`, C1 on the align commit, and C2/C3 limited
to the todo's directories until STYLE-04 widens them to the whole tree.

## Open questions

## Owner answers (2026-11-20, owner accepted the proposals)
- Q1 tooling: T3. No formatter now; `.clang-format` only through a later planning if drift shows up.
- Q2 doc-comment check: C3 script exists (heuristic); false positives are fixed in the script, never by weakening the rule.
- Q3 ctest: C2 and C3 become ctest entries in STYLE-04; until then they are run by hand per todo.
- Q4 comment language: English only.
- Q5 tests: comments only where setup or expectation is non-obvious; no comment required per `TEST_CASE`.

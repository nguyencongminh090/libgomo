---
id: clang-format
type: planning
size: L
status: approved
owner: claude
created: 2026-11-23
supersedes: []
links: [code-style-normalization]
---
# Add a `.clang-format`
Discussion for an L-size feature (toolchain change). Open questions must be empty to approve. After approval it is frozen; split todos (each at most M) carry `planning: clang-format`.

## Goal
Add a `.clang-format` at the repo root so the style in `.claude/rules/coding-style.md` (4-space indent, 100 columns, aligned
consecutive assignments/declarations/trailing comments) can be applied and checked by a tool, not by hand. Success: running the
formatter on `include/ src/ tests/` changes nothing (or only changes the owner has accepted), and the build/test result is unchanged.

## Non-goals
- No logic, rename, reorder or API change. `third_party/` is never formatted.
- No change to naming, comment rules or the doc-comment checks (`scripts/check-doc-comments.sh`, `scripts/check-line-length.sh`).
- Scripts (`.mjs`, `.sh`), CMake files and docs are out of scope.

## Constraints
- `clang-format` is not installed on this machine (checked 2026-11-23). Adding it is a new toolchain dependency.
- Project rule: alignment is a separate pass run only on `/align`; edits inside aligned blocks are written unaligned. A
  formatter-on-save or a format check in `ctest` would contradict that rule unless the rule is changed (owner decision).
- Planning `code-style-normalization` (closed) left the whole tree hand-aligned under rules that clang-format cannot express:
  - a block is not aligned when one outlier pushes the column more than about 20 characters right;
  - alignment never crosses a blank line or an unrelated line.
  clang-format has `AlignConsecutive*` with `AcrossEmptyLines`/`AcrossComments` switches but no outlier threshold, so the first
  full run will produce a diff in blocks that were deliberately left unaligned.
- Reformat must stay reviewable: a formatter run is its own commit (`style: clang-format`), whitespace-only, checked with
  `git diff -w` as in C1 of the previous planning.
- Version drift: clang-format output changes between major versions; the version must be pinned or recorded (the project
  checks with g++ 15, so clang-format 20 or newer is likely).
- Doctest `TEST_CASE` macros and `///<` trailing comments need care (`AlignTrailingComments`, `MacroBlockBegin`/`StatementMacros`).

## Options
Scope of the file:
- S1. Alignment-only config: `DisableFormat: false` with `BasedOnStyle` set to keep the current layout and only enable
  `AlignConsecutiveAssignments`, `AlignConsecutiveDeclarations`, `AlignTrailingComments`, `ColumnLimit: 100`, `IndentWidth: 4`.
  Smallest diff, closest to the existing rule.
- S2. Full style (Google/LLVM/Mozilla-derived base plus the options above). Reformats braces, includes order, wrapping; large
  diff, may fight existing code layout (for example `if` bodies, lambda layout).
- S3. A config used only as a tool for `/align` (not enforced anywhere), kept next to the rule.

Enforcement:
- E1. Config only. Contributors run `clang-format -i` by hand. No check.
- E2. Config plus `scripts/check-format.sh` (clang-format `--dry-run -Werror` on `include/ src/ tests/`), run by hand and listed in
  `CLAUDE.md` Commands.
- E3. E2 plus a ctest entry (like the other check scripts). Needs clang-format on every dev machine and CI; conflicts with the
  "align only on `/align`" rule because a stray unaligned edit would fail the test.

Handling the outlier rule:
- O1. Accept clang-format alignment as the new truth; the one-off reformat commit realigns outlier blocks and the 20-character
  exception in the rule is dropped (rule change).
- O2. Keep the exception: mark such blocks with `// clang-format off` / `on` (cost: noise in the code).
- O3. Turn alignment options off for assignments (keep trailing comments only), so the rule's outlier exception no longer has
  a counterpart in the tool.

Split (each todo at most M, build green after each):
1. FMT-01: spike or note, then add `.clang-format` (chosen scope) and record the pinned clang-format version.
2. FMT-02: one `style: clang-format` commit applying it to `include/ src/ tests/`, verified with `git diff -w` and
   `ctest --preset default`.
3. FMT-03 (only if E2/E3): `scripts/check-format.sh` and the `CLAUDE.md`/rule updates.

## Decision
Approved by owner 2026-11-23, in two phases (the owner named two options per question; read as sequential):
- Phase 1: S1 + E2 + O1. Alignment-only `.clang-format`, `scripts/check-format.sh` run by hand, clang-format alignment accepted as
  the new truth (the 20-character outlier exception in `.claude/rules/coding-style.md` is dropped).
- Phase 2: S2 + E3 + O2. Full style, the check becomes a ctest entry, and blocks that must stay as they are are fenced with
  `// clang-format off` / `on`. Phase 2 starts only after phase 1 is done; it re-uses the same file and replaces the phase 1
  scope, so it needs its own measurement of the reformat diff first.
Split (each todo at most M, build green after each; all carry `planning: clang-format`):
1. FMT-01: install clang-format, record the version, add the alignment-only `.clang-format`, dry-run over `include/ src/ tests/` and
   record the size of the diff in an audit.
2. FMT-02: one `style: clang-format` commit applying it (`git diff -w` empty apart from alignment, `ctest --preset default` green),
   plus `scripts/check-format.sh` and the updates to `CLAUDE.md` Commands and the coding-style rule.
3. FMT-03 (phase 2): full-style config and `// clang-format off` fences, measured first.
4. FMT-04 (phase 2): the format check as a ctest entry and the rule rewrite ("align only on `/align`" no longer applies).
Acceptance per todo: `ctest --preset default`, `scripts/check-core-deps.sh`, `scripts/check-format.sh` from FMT-02 on.

## Open questions

## Owner answers (2026-11-23)
- Q1 scope: S1, then S2 in phase 2.
- Q2 enforcement: E2, then E3 in phase 2.
- Q3 outliers: O1, then O2 in phase 2.
- Q4 install: the owner allows installing clang-format; the agent installs it in FMT-01 and records the version.
- Q5 tests: delegated to the agent. Decision: `tests/` is in scope from the first run, because `TEST_CASE` is only a macro line to
  clang-format; FMT-01 measures it, and if `tests/` produces a noisy diff the reformat is split per directory.

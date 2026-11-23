---
id: FMT-04-format-check-ctest
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: []
planning: clang-format
---
# Phase 2: format check as a ctest entry and rule rewrite

## Scope
- Add the format check to ctest (like the other check scripts, see `scripts/check-ctest-coverage.sh`).
- Rewrite the coding-style rule: the formatter now owns alignment, `/align` and the 'edit unaligned' rule are retired or redefined.

## Don't
- No code change beyond the CMake test entry. Do not weaken the check to pass.

## Done-when
Features `FMT-04-*` in `docs/features.json` (`harness-track FMT-04`).

## Evidence

## Links
`features/clang-format/planning.md`

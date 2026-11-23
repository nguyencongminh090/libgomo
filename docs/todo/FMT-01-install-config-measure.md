---
id: FMT-01-install-config-measure
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: []
planning: clang-format
---
# Install clang-format, add the alignment-only config, measure the diff

## Scope
- Install clang-format and record its version (pin it in the audit and in `CLAUDE.md` Commands).
- Add `.clang-format`: 4-space indent, ColumnLimit 100, AlignConsecutiveAssignments/Declarations, AlignTrailingComments, nothing else (planning phase 1, S1).
- Dry-run it over `include/ src/ tests/` (never `third_party/`), write the size of the diff per directory into an audit in `docs/audit/`.

## Don't
- Do not apply the formatter to the tree (that is FMT-02). No code, comment or logic change.

## Done-when
Features `FMT-01-*` in `docs/features.json` (`harness-track FMT-01`).

## Evidence

## Links
`features/clang-format/planning.md`

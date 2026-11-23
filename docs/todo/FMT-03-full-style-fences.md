---
id: FMT-03-full-style-fences
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: []
planning: clang-format
---
# Phase 2: full-style config with clang-format off fences

## Scope
- Start only after FMT-02 is done. Measure the full-style (S2) reformat diff first and record it in an audit.
- Extend `.clang-format` to full style; fence blocks that must stay as written with `// clang-format off` / `on` (option O2) and a one-line why.

## Don't
- No logic change. Do not add fences to dodge the check without a reason in the comment.

## Done-when
Features `FMT-03-*` in `docs/features.json` (`harness-track FMT-03`).

## Evidence

## Links
`features/clang-format/planning.md`

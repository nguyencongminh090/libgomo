---
id: CORE-08-rename-cpp-integration
type: todo
size: M
status: backlog
owner: codex
created: 2026-11-23
supersedes: []
links: [rename-libgomo, CORE-07-plan-libgomo-rename]
planning: rename-libgomo
---
# Rename the C++ namespace and public headers to gomo

## Scope

Move the public tree from `include/pygomo/` to `include/gomo/`. Update include paths,
namespace declarations, and qualified references in the library and tests. Point the
existing dependency script at the new tree. Keep the same module boundaries and dependency
checks. The seven style acceptance commands that contain header paths need corresponding
updates with owner authorization.

## Done-when

- A fresh default build and the full CTest suite pass with the renamed headers and namespace.
- `scripts/check-core-deps.sh` still enforces a protocol- and I/O-free core and a protocol-blind
  transport. The line-length and doc-comment checks pass against the relocated headers.
- The acceptance entries for `CORE-08` are approved and pass in `docs/features.json`.

## Evidence

Pending implementation.

## Links

[Approved plan](../../features/rename-libgomo/planning.md)

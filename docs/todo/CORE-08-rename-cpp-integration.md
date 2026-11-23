---
id: CORE-08-rename-cpp-integration
type: todo
size: M
status: active
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

- Relocated all 25 public headers to `include/gomo/` and migrated the namespace and include
  references in the library, tests, and test support. The dependency script uses the new
  paths with the same checks.
- Configured the default preset in a fresh temporary build directory and built with g++
  15.2.0 and warnings as errors. CTest passed all 119 entries, including all seven live entries
  with the local Rapfi V2 and V3 binaries. All CTest names and doctest filters matched the
  baseline.
- A standalone CMake consumer linked `pygomo::gomocup` via `add_subdirectory` and used the new
  headers and namespace to create a position, construct the adapter, and parse a move.
- The core dependency check and all seven style commands passed with the relocated paths.
  Temporary negative cases verified that the dependency and style scripts still reject
  forbidden dependencies and style violations.
- Updated four Markdown link targets to point to the relocated headers.
- The acceptance entries and the seven path-only changes in `docs/features.json` await owner
  approval.

## Links

[Approved plan](../../features/rename-libgomo/planning.md)

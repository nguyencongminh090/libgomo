---
id: CORE-09-rename-cmake-test-identifiers
type: todo
size: M
status: backlog
owner: codex
created: 2026-11-23
supersedes: []
links: [rename-libgomo, CORE-08-rename-cpp-integration]
planning: rename-libgomo
---
# Rename the CMake project, targets, and test settings

## Scope

After CORE-08, rename the CMake project to `libgomo`, the targets and executables to
`gomo_*`, the aliases to `gomo::`, and the option, fixture definitions, and live settings
to `GOMO_*`. Keep the existing CTest names, doctest filters, and dependency graph, including
the `UNIX` guard for process support. Update the warning and live acceptance commands
listed in the plan with owner authorization.

## Done-when

- Clean `default` and `tsan` builds pass their CTest suites with the same registered test names
  and filters. The default preset still enables warnings as errors.
- A standalone `add_subdirectory` consumer verifies the new aliases and links `gomo::gomocup`.
  Its executable constructs the adapter and transport and creates a `Position`.
- The `GOMO_RAPFI`, `GOMO_RAPFI_V2`, and `GOMO_RAPFI_V3` settings select the expected engines.
  Record which live checks ran and which skipped when a binary was unavailable.
- The acceptance entries for `CORE-09` are approved and pass in `docs/features.json`.

## Evidence

Pending implementation.

## Links

[Approved plan](../../features/rename-libgomo/planning.md)

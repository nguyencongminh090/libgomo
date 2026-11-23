---
id: CORE-09-rename-cmake-test-identifiers
type: todo
size: M
status: active
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

- The CMake project is `libgomo`. The library targets, archives, and test executables use
  `gomo_*`, the aliases use `gomo::`, and the warning option, fixture definitions, and live
  variables use `GOMO_*`. A comparison of the CMake commands before and after the rename
  verified the dependency graph and the `UNIX` guards. `scripts/check-core-deps.sh` passed.
- Fresh `default` and `tsan` preset builds with g++ 15.2.0 each passed all 119 CTest entries.
  In both presets, all 119 names, commands, and properties matched the baseline after accounting
  for the renamed executables and temporary build directories. The cache and compiler commands
  confirmed warnings as errors, and the two existing TSAN-specific exceptions remained scoped
  to the `tsan` preset.
- A standalone `add_subdirectory` consumer linked `gomo::gomocup`, verified all eight aliases
  on Unix, and confirmed the previous CMake aliases and targets were absent. The executable
  constructed a `StreamTransport` and `GomocupEngine`, created a 15x15 `Position`, and played a move.
- With `GOMO_RAPFI` and `GOMO_RAPFI_V2` pointing to Rapfi V2 and `GOMO_RAPFI_V3` pointing to the
  Rapfi V3 dev build, `gomo2_live`, `gomo2_stop`, `gomo2_close`, `gomo2_deadline`, `gomo7_live`,
  `ext09_live`, and `ext10_live` all ran and passed. All seven skipped when the new settings
  were unset, including when the old settings were present.
- On the first `tsan` run, `ext10_live` timed out waiting for the V3 `ABOUT` response after
  forcing a search to stop. This matches the failure mode described in
  [AUD-08](../audit/AUD-08-rapfi-extension-findings.md). The full suite passed on rerun with the
  same settings and assertions.
- Approval for the CORE-09 acceptance entries and the renamed checks in `docs/features.json`
  is pending.

## Links

[Approved plan](../../features/rename-libgomo/planning.md)

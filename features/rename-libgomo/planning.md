---
id: rename-libgomo
type: planning
size: L
status: approved
owner: codex
created: 2026-11-23
supersedes: []
links: [CORE-07-plan-libgomo-rename]
---
# Rename PyGomo to libgomo

## Goal

Adopt the owner-selected name `libgomo` across the public project and its C++ integration.

## Non-goals

- Changes to engine behavior, supported protocols, or module boundaries.
- Changes to repository hosting or historical records.

## Constraints

- Keep every implementation todo buildable and preserve the CTest names, doctest filters,
  protocol fixtures, and test assertions. Verify the migration from fresh build directories.
- Keep the CMake dependency graph. Core links no other library and uses no I/O, thread,
  process, transport, or protocol headers. Transport stays independent of the core and
  protocol names. Process support stays conditional on `UNIX`.
- Use `add_subdirectory` for consumer verification. The checkout has no CMake install or
  export rules, release tags, or configured remotes. The inventory covers this checkout.
- Keep dated ADRs, audits, completed todo evidence, reports, and archived material as
  historical records. Update current instructions and examples that consumers will run.

## Integration inventory

| Surface | Current | Approved |
|---|---|---|
| Project name and CMake `project()` | `PyGomo` | `libgomo` |
| C++ namespace | `pygomo`, `pygomo::parse`, `pygomo::encode` | `gomo`, `gomo::parse`, `gomo::encode` |
| Public headers | `include/pygomo/`, `#include <pygomo/...>` | `include/gomo/`, `#include <gomo/...>` |
| CMake aliases | `pygomo::{core,transport,parse,engine,gomocup,gtp,katagomo,process}` | `gomo::` with the same components |
| Concrete CMake targets and static archives | `pygomo_<component>`, `libpygomo_<component>.a` on Linux | `gomo_<component>`, `libgomo_<component>.a` |
| Build options and helpers | `PYGOMO_WARNINGS_AS_ERRORS`, `pygomo_enable_warnings` | `GOMO_WARNINGS_AS_ERRORS`, `gomo_enable_warnings` |
| Test targets and fixture definitions | `pygomo_*_tests`, `pygomo_smoke`, `PYGOMO_FIXTURE_DIR`, `PYGOMO_GTP_FIXTURE_DIR` | `gomo_*_tests`, `gomo_smoke`, `GOMO_FIXTURE_DIR`, `GOMO_GTP_FIXTURE_DIR` |
| Live test environment | `PYGOMO_RAPFI`, `PYGOMO_RAPFI_V2`, `PYGOMO_RAPFI_V3` | `GOMO_RAPFI`, `GOMO_RAPFI_V2`, `GOMO_RAPFI_V3` |

`CMakeLists.txt`, `CMakePresets.json`, and `tests/CMakeLists.txt` define the build names.
`tests/live/rapfi_live_test.cpp` reads the Rapfi settings. `scripts/check-core-deps.sh`
points to the old header tree. Current examples and commands appear in `README.md`,
`AGENTS.md`, `CLAUDE.md`, `docs/handoff.md`, and the developer guide. The architecture
Markdown, Draw.io source, rendered PNG, and developer-guide PDF also carry the old names.

Fourteen existing acceptance commands use integration names that will change.

- `STYLE-01-scripts`, `STYLE-01-lines`, `STYLE-01-docs`, `STYLE-02-lines`, `STYLE-02-docs`,
  `STYLE-03-lines`, and `STYLE-03-docs` refer to `include/pygomo/`.
- `TSAN-01-scope` checks the warning option. `GOMO-03-live`, `DEAD-03-main`, `DEAD-04-live`,
  `GOMO-07-live`, and `LIVE-01-live` use the Rapfi environment variables.
- `CORE-03-edges` checks `links pygomo::core` in Draw.io.

With owner authorization, update the commands and descriptions that use these integration
names and rerun the same checks. The guide checks' `/tmp/pygomo-guide` argument is an arbitrary
scratch path and remains usable.

## Options

1. **Direct migration (approved).** Consumers update include paths, namespace qualifiers,
   target references, and settings together and rebuild the library and their application.
   Use a fresh consumer build directory so old CMake cache options and archive paths are
   replaced. The README gives the exact substitutions for source and CI jobs.
2. **One-release source compatibility.** Ship forwarding headers, a namespace alias, and
   legacy CMake aliases. Read old warning and Rapfi settings as a fallback when their new
   names are unset. Verify old and new consumer snippets and specify the removal release
   before coding. Consumers would still rebuild. A namespace alias cannot support code that
   reopens `namespace pygomo`, and it cannot preserve already-linked C++ symbols.

## Decision

The owner approved direct migration on 2026-11-23. The project name is `libgomo`, with
`gomo`, `gomo/`, and `gomo::` for C++ and CMake integration. Consumers update the names and
rebuild from source in a fresh build directory. The checkout shows a source-based integration
without a tagged release. [Compatibility decision](../../docs/audit/AUD-09-libgomo-migration.md)

## Implementation slices

The consumer-facing migration is complete when all three slices pass.

1. **CORE-08. C++ namespace and headers.** Move the header tree, rename namespaces and qualified
   uses across the library and test sources, and update the dependency script and affected
   style acceptance commands with owner authorization. Preserve the existing module layout.
   Verify with a fresh default build, the full CTest suite, the style checks whose paths
   change, and `scripts/check-core-deps.sh`.
2. **CORE-09. CMake and test identifiers.** Rename the project, targets, option, test definitions,
   and live-test variables. Update the affected warning and live acceptance commands with
   owner authorization. Keep the `default` and `tsan` preset names. Configure both in
   fresh build directories under paths without spaces, build and run their CTest suites,
   and confirm the default build still enables warnings as errors. Compare the configured
   CTest names and filters with the pre-rename run. Use available V2 and V3 Rapfi binaries
   to exercise the live variables. Record live checks as skipped when their engine is unavailable.
3. **CORE-10. Documentation and consumer migration.** Update the README, contributor
   commands, architecture source and rendered PNG, and the developer guide source and rebuilt
   PDF. Update the affected diagram acceptance command with owner authorization. Document the
   source and settings substitutions and the clean rebuild requirement. Verify that the examples
   compile, the guide builds, the diagram label and PNG agree, and the harness passes.

For the second slice, build and run a standalone CMake project that adds a quoted checkout
path with `add_subdirectory`, uses `gomo/` headers and `gomo::` symbols, and links
`gomo::gomocup`. Instantiate a `StreamTransport` and `GomocupEngine` and create a `Position`
to exercise the adapter's transitive dependencies without a live engine. Assert that each
new CMake alias exists and check `gomo::process` on Unix. Under direct migration, verify
that the old include tree and CMake aliases are absent and that a search of maintained
source, build, and test files finds no old identifiers. Allow explicit old-to-new mappings
in migration documentation.

## Open questions

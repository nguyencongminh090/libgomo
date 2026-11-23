---
id: CORE-10-document-libgomo-migration
type: todo
size: M
status: active
owner: codex
created: 2026-11-23
supersedes: []
links: [rename-libgomo, CORE-09-rename-cmake-test-identifiers, AUD-09-libgomo-migration]
planning: rename-libgomo
---
# Document the libgomo consumer migration

## Scope

After CORE-09, update the README example and migration instructions, current contributor
commands, developer guide source and PDF, and architecture Markdown, Draw.io, and PNG.
Show the old and new integration names and the clean rebuild required by the approved
policy. Update the `CORE-03-edges` diagram check with owner authorization.

## Done-when

- Updated consumer examples compile against the renamed interface. README substitutions
  cover headers, namespaces, CMake targets, warning options, and live test settings.
- The guide rebuilds and the architecture source and PNG show the same renamed targets.
  The diagram check still verifies that the parser links the core.
- The harness and the approved acceptance entries for `CORE-10` pass.

## Evidence

- Extracted the README and guide listings and built them in a fresh standalone CMake project
  with `add_subdirectory` pointing to the checkout path containing spaces. Rapfi V2 returned
  `best move: 9,6` in the README example and `best move 8,5` in the guide tutorial. The
  asynchronous guide example printed `7,7 visits 120` and `best move 7,6` against a fake GTP
  engine. The guide snippets for extension points and process setup also compiled and linked.
- Rebuilt the developer-guide PDF with `latexmk`. The log had no undefined references or
  overfull boxes. Extracted PDF text showed the `libgomo` title, `gomo::` aliases and `GOMO_`
  settings. The README lists the source and CI substitutions and the fresh build requirement.
- Compared all 109 Draw.io cells with the source embedded in the PNG. The parser-to-core
  edge shows `links gomo::core` in the source and rendered image. All 14 direct dependencies
  in the Mermaid flowchart match CMake after expanding the grouped adapters.
- Built the `default` preset in a clean directory with warnings as errors. The smoke check,
  direct `gomo_core_tests` example and all 119 CTest entries passed, including the seven live
  entries with Rapfi V2 and V3. The harness reported zero errors.
- The saved `CORE-03-edges` command exits 1 on its legacy label. The same command with
  `gomo::core` exits 0. Updating the saved command and adding the CORE-10 acceptance entries
  await owner approval under the acceptance rules.

## Links

[Approved plan](../../features/rename-libgomo/planning.md) ·
[Compatibility decision](../audit/AUD-09-libgomo-migration.md)

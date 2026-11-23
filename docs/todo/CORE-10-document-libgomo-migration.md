---
id: CORE-10-document-libgomo-migration
type: todo
size: M
status: backlog
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

Pending implementation.

## Links

[Approved plan](../../features/rename-libgomo/planning.md) ·
[Compatibility decision](../audit/AUD-09-libgomo-migration.md)

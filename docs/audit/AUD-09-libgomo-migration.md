---
id: AUD-09-libgomo-migration
type: audit
size: M
status: active
owner: codex
created: 2026-11-23
supersedes: []
links: [CORE-07-plan-libgomo-rename, rename-libgomo]
---
# Direct migration to libgomo

## Context

PyGomo exposes C++ symbols, public headers, and CMake targets. A standalone consumer
configured with `add_subdirectory` built and ran against the existing names. The checkout
has no CMake install or export rules, release tags, or configured remote.

## Decision

On 2026-11-23 the owner approved a direct migration to the `libgomo` project name and the
`gomo` namespace, `gomo/` headers, `gomo::` CMake aliases, and `GOMO_` settings. Consumers
update their references and rebuild the library and application in a fresh build directory.

A transition release would add forwarding headers, a namespace alias, CMake aliases, and
fallbacks for old settings. The checkout shows a source-based integration, so one set of
public names is sufficient for the approved migration. The README will list the exact
substitutions. [Approved plan](../../features/rename-libgomo/planning.md)

## Status

Active.

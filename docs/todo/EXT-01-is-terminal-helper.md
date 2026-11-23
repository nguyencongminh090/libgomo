---
id: EXT-01-is-terminal-helper
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# Core: isTerminal(SearchEvent) replaces four copies of the terminal test

## Scope
Add `isTerminal(const SearchEvent&)` in `include/pygomo/core/search_event.hpp` (Info, Message, Unparsed are not terminal; everything else is). Replace the copies in `src/engine/blocking_search.cpp`, `src/engine/deadline_engine.cpp`, `src/katagomo/katagomo_engine.cpp`, `src/gomocup/gomocup_engine.cpp`. Pure refactor.

## Don't
No new event type, no behaviour change, no edit of existing tests.

## Traps
ADR-01: a new type that is missed by one copy ends the search at its first line. `GtpEngine` has its own terminal logic (reply based); check it too and leave it if it is not the same test.

## Done-when
Features `EXT-01-*` in `docs/features.json` (`harness-track EXT-01`).

## Evidence
2026-11-23. `isTerminal` added as a constexpr free function in `search_event.hpp`; the four copies
(blocking_search, deadline_engine, katagomo_engine, gomocup_engine) now call it. `GtpEngine` left alone:
it ends a search on the genmove reply (`finish()`), not by event type. `tests/live/rapfi_live_test.cpp`
keeps its own copy (tests are not edited).
- `ctest --preset default -R '^ext01_' --no-tests=error`: 1/1 passed.
- Clean build with warnings as errors; `ctest --preset default`: 84/84 passed (live tests skipped, no binary
  configured). check-core-deps, check-format, check-doc-comments, check-line-length, check-ctest-coverage: OK.

## Links
`features/extensible-engine-commands/planning.md`

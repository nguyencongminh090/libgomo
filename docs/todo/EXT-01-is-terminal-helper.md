---
id: EXT-01-is-terminal-helper
type: todo
size: M
status: backlog
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
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

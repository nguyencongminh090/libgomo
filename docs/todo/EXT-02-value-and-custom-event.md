---
id: EXT-02-value-and-custom-event
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# Core: Value and CustomEvent

## Scope
Add `Value = std::variant<bool, std::int64_t, double, std::string, Coord, Move, std::vector<Coord>, std::vector<Move>>` and a default `format(Value)` in core (data only). Add `CustomEvent{tag, fields}` (fields: list of key/Value) to `SearchEvent`; it is not terminal.

## Don't
No parser or adapter produces CustomEvent yet. No I/O header in core.

## Traps
Add the new type only after EXT-01. Check the `std::visit` and `holds_alternative` users in tests and src still compile and do not treat CustomEvent as terminal.

## Done-when
Features `EXT-02-*` in `docs/features.json` (`harness-track EXT-02`).

## Evidence
2026-11-23. `Value` + `format(Value)` in `core/value.hpp` (src/core/value.cpp, std::format only), `CustomEvent{tag, fields}` added to `SearchEvent`; `isTerminal` treats it as non-terminal. Tests `ext02_*` cover core, `blocking_search`, `DeadlineEngine` and `GomocupEngine` (via a custom parser from EXT-03). `KataGomoEngine` has no parser hook until EXT-04, so no CustomEvent can reach it yet; it only calls `isTerminal`, which is tested. `check-core-deps` OK, suite 88/88 (live skipped).

## Links
`features/extensible-engine-commands/planning.md`

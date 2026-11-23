---
id: EXT-02-value-and-custom-event
type: todo
size: M
status: backlog
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
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

---
id: PORT-01-engine-port-event-sink
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# EnginePort and EventSink

## Scope
- Core, header-only, no I/O: `EventSink` (`onEvent(const SearchEvent&)`), `EnginePort`
  (`setRules` -> `expected<void, EngineError>`, `supports(Rules)`, `startSearch(Position, Limits,
  EventSink&)`, `stop()`, `sendRaw(line)`), `EngineError` (`Unsupported`, `Busy`, `NotReady`,
  `TransportFailed`).
- New library `pygomo_engine` (core + threads): blocking `search(EnginePort&, Position, Limits)`
  built on `startSearch`. It waits for a terminal event (best move, swap, pass, error).
  It lives outside core because it needs a mutex and condition variable, which core may not include.

## Don't
- No adapter, no protocol knowledge, no transport dependency.

## Done-when
`docs/features.json`, `"todo": "PORT-01"`.

## Evidence
2026-11-19: port_sink, port_contract, port_blocking passed; full suite 14/14 default and tsan; check-core-deps OK. Blocking helper placed in pygomo_engine (needs mutex, forbidden in core).

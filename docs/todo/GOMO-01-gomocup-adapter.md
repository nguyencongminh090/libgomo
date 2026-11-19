---
id: GOMO-01-gomocup-adapter
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# Gomocup adapter

## Scope
- Library `pygomo_gomocup` (core, parse, transport). `GomocupEngine` implements `EnginePort` over
  a `Transport&` (not owned) and parses lines with `parseGomocupLine`.
- `startSearch` writes `START w`, `INFO RULE n`, `INFO MAX_MOVES`, the set limits
  (`TIMEOUT_TURN`, `TIMEOUT_MATCH`, `MAX_DEPTH`, `MAX_NODE`), then `BOARD` + `x,y,flag` lines +
  `DONE` (flag 1 = stone of the side to move, 2 = the other; empty board: `BEGIN`).
- Rules table from the planning. `setRules` validates and stores; sent with the next search.
- `stop()` sends `YXSTOP` (Yixin extension; Rapfi still prints a move afterwards).
- A best move outside the board or on a stone becomes an error event.
- Any `ERROR` line ends the search (terminal), as the port says.

## Traps
- Lines that arrive while no search runs (engine banner) are dropped: not yet exposed.
- INFO values from an earlier search persist in the engine; only set limits are sent.
- Event sink is called outside the adapter's lock.

## Done-when
`docs/features.json`, `"todo": "GOMO-01"`.

## Evidence
2026-11-19: gomo_sequence/rules/events/stop/closed/rect passed; suite 20/20 default and tsan; 20 repeats of gomo+port under tsan clean; check-core-deps OK. Tested on a fake transport only; no real engine run yet (nothing built).

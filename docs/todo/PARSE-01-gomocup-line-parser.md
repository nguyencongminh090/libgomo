---
id: PARSE-01-gomocup-line-parser
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# Gomocup line parser

## Scope
- New library `pygomo_parse` (links `pygomo_core` only; no streams, threads or transport).
  `parseGomocupLine(std::string_view) -> SearchEvent`, pure and total (never throws).
- `ERROR <text>` -> `ErrorEvent`. `SWAP` -> `SwapEvent`. `x,y` -> `BestMoveEvent` (0-based,
  same origin as `Coord`). `MESSAGE` lines in the UCI-like form (`depth 10-12 ev 34 n 1.2M tm 900
  pv ...`) -> `InfoEvent` (depth, eval, nodes, elapsed, pv as `x,y` or `H8` labels).
- Everything else, including plain `MESSAGE`/`DEBUG` text and two-move replies, becomes
  `UnparsedEvent` with the original line (core has no message event; adding one is a core change,
  recorded for the owner, not done here).
- Source of line shapes: Rapfi `command/gomocup.cpp`, `search/searchoutput.cpp`, `core/iohelper.*`
  (read only; nothing built, no captured fixtures yet).

## Don't
- No board-size checks (the adapter knows the size), no I/O, no process, no GTP.
- Do not touch `pygomo_core`.

## Done-when
Acceptance lives in `docs/features.json` (`"todo": "PARSE-01"`); `harness-track PARSE-01` lists them.

## Evidence
2026-11-19, g++ 15 default preset, warnings as errors: `ctest --preset default -R parse_<messages|move|info|unknown>` each passed; full suite 11/11; `scripts/check-core-deps.sh` OK.
Decision noted for the owner: core has no message event, so plain `MESSAGE`/`DEBUG` text and the two-move reply (`x,y x,y`) map to `UnparsedEvent`. Rapfi `MESSAGE` PV in NORMAL mode uses `|`-separated text and is not parsed (only the UCI-like mode is).

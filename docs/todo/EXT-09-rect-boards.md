---
id: EXT-09-rect-boards
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# Rectangular boards: RECTSTART in GomocupEngine

## Scope
`GomocupEngine` sends `RECTSTART w,h` instead of `START n` for a non-square board. `GtpEngine` accepts a non-square board (it already sends `boardsize w h`). `KataGomoEngine` keeps `Unsupported` and says why in its doc comment. Add fixtures and a live test against Rapfi V3 (skipped when there is no binary).

## Don't
Do not change square-board lines.

## Traps
Rapfi under `FLIPY_X` coordinate mode swaps w and h (`gomocup.cpp` RECTSTART); no fixture covers `coord_conversion_mode` != none (handoff). An engine without RECTSTART prints ERROR and the search stays open until DeadlineEngine fires.

## Done-when
Features `EXT-09-*` in `docs/features.json` (`harness-track EXT-09`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

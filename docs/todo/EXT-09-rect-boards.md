---
id: EXT-09-rect-boards
type: todo
size: M
status: done
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
2026-11-23, Sonnet subagent. `ctest -R '^ext09_'` 4/4 (live skipped without env); full suite 113/113 on master. Live: `PYGOMO_RAPFI_V3=.../Rapfi_V3_PTR/dev/rapfi/gomoku-portal-ui-distribute/pbrain-rapfi ctest -R ext09_live` passed (RECTSTART accepted, legal move on 15x10), cwd = binary dir for config.toml. Existing tests changed because the approved behaviour changed: `gomo_rect` now expects RECTSTART; the non-square refusal line in `ext07_default` removed. GtpEngine needed no code change.

## Links
`features/extensible-engine-commands/planning.md`

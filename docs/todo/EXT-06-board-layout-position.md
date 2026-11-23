---
id: EXT-06-board-layout-position
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# Core: BoardLayout in Position (WALL, Torus, Portal)

## Scope
Add `BoardLayout{bool torus; vector<Coord> blocked; vector<pair<Coord,Coord>> portals; map<string,Value> extensions}`; `Position::create(w, h, layout = {})`; `play()` returns `PositionError::Blocked` on a blocked cell; `moves()` and `sideToMove()` unchanged; `operator==` compares the layout. A blocked or portal cell off the board is `OutOfBounds` at `create`. List every `switch` on `PositionError` and fix it.

## Don't
Core does not check what the engine checks (torus size, portal on a rectangular board, Renju on a torus).

## Traps
`Blocked` goes last in the enum. `occupied()` stays about stones.

## Done-when
Features `EXT-06-*` in `docs/features.json` (`harness-track EXT-06`).

## Evidence
2026-11-23, Opus subagent. `ctest -R '^ext06_'` pass; check-core-deps OK; full suite 93/93 on master. No `switch` on PositionError in the repo (only returns and test comparisons); guide listing updated. Open: `BoardLayout ==` is order-sensitive (walls in another order compare unequal); duplicate walls accepted.

## Links
`features/extensible-engine-commands/planning.md`

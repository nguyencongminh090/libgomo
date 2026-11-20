---
id: KATA-01-katagomo-gom-adapter
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: engine-protocol-port
---
# KataGomoEngine: adapter for KataGomo `gom` mode (source-derived)

## Scope
- New `KataGomoEngine` (`pygomo_katagomo` library, core + parse + transport), separate from `GomocupEngine` (owner's choice).
- `setRules` validates, stores. Supported: `{Freestyle|Standard|Renju} x FreeOpen`; swap openings Unsupported.
- `startSearch` writes: `START w`, `FREESTYLE`/`STANDARD`/`RENJU`, `maxmoves N` (if set), `INFO timeout_turn MS`,
  `kata-set-param maxVisits N` (maxNodes), then `BOARD` + `x,y` lines in play order + `DONE`. START resets the board, so the settings follow it. No `BEGIN` command exists: an empty board is `BOARD`, `DONE`.
  `maxDepth` -> Unsupported. `matchTime` -> Unsupported (owner, 2026-11-20).
- Reply `x,y` -> best move (via `parseGomocupLine`); `MESSAGE ...` -> `UnparsedEvent`; `ERROR ...` ends the search.
- Success of setup commands prints nothing; the `OK` after `START` is dropped, not an event.
- `stop()` is a no-op (single-thread loop, same limit as GtpEngine).

## Don't
- No change to `GomocupEngine`, core, parser or transport. No GTP `genmove` path here (that is GTP-03).

## Traps
- Facts come from reading `gomprotocol.cpp` only; no KataGomo engine has been run. Tested on a fake transport.
- Limits persist in the engine between searches (same as GomocupEngine).

## Done-when
Acceptance in `docs/features.json` (`"todo": "KATA-01"`), after owner approval.

## Evidence
2026-11-20: owner approved 5 features and matchTime = Unsupported. `ctest --preset default` 43/43 (kata_* 4 new; gomo2_* skipped), `ctest --preset tsan` 43/43, `check-core-deps.sh` OK. Source re-read from KataGomo Gom2026 (START resets board; no BEGIN; BOARD ignores a third field). Fake transport only; no KataGomo run.

## Links
`docs/notes/2026-11-20-kata-gomprotocol-findings.md`, `docs/todo/GOMO-01-gomocup-adapter.md`, `docs/todo/GTP-02-gtp-adapter.md`.

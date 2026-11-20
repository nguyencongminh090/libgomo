---
id: 2026-11-20-kata-gomprotocol-findings
type: note
size: S
status: active
owner: claude
created: 2026-11-20
supersedes: []
links: []
---
# KataGomo `gomprotocol.cpp`: can GomocupEngine drive it?
Temporary scratch for an S-size task. Delete it, or promote it to a todo or fix-log. Never keep decisions here.

## What
Read KataGomo `Gom2026` @ `9a0e65c`, `cpp/command/gomprotocol.cpp` (3016 lines; read only, nothing built or run),
and compared it with what `GomocupEngine` sends and expects. Entered by `katago gom`, or by default in a
`FORGOMOCUP` build (`cpp/main.cpp:94,161`).

## Outcome
Facts from the source (no engine was run):
- One loop accepts both GTP commands (`play`, `genmove`, `FREESTYLE`/`STANDARD`/`RENJU`, `maxmoves`,
  `kata-set-param`...) and Gomocup commands (`START`, `RESTART`, `INFO`, `BOARD`..`DONE`, `SWAP2BOARD`, `TURN`,
  `ABOUT`, `END`, `setswap2time`). Gomocup replies are plain lines; errors are `ERROR <text>`; no `=`/blank framing.
- `START n` or `START w:h` / `START w h` replies `OK`. `BOARD` reads `x y` lines (commas become spaces) until
  `DONE`, alternating colours from Black and ignoring a third field, then searches and replies `x,y`
  (`Location::getX/getY`, 0-based, same as `parseGomocupLine`). `TURN x y` plays and replies likewise.
- `INFO` understands only `time_left` and `timeout_turn` (lowercase, case-sensitive; milliseconds). Any other
  `INFO` key, including `INFO RULE n`, `MAX_DEPTH`, `MAX_NODE`, `MAX_MOVES`, is accepted and ignored.
  Rules are set only by the GTP-style commands `FREESTYLE`/`STANDARD`/`RENJU` and `maxmoves N`.
- `MESSAGE` lines: banner at start (`MESSAGE Loaded model ...`); during `genMove` only if `ogsChatToStderr`:
  `MESSAGE Visits N Winrate P% Drawrate P% Time T PV ...` (not the UCI-like or Rapfi normal shapes, so it parses as
  `UnparsedEvent`). `SWAP2BOARD` prints `MESSAGE White winrate = ...` and may reply `SWAP` or two moves.
- Same single-thread loop as GTP: a line sent during a search is read only after the reply; `YXSTOP` is not a
  command (`ERROR unknown command`, harmless but useless) and `stop` only affects ponder/analyze.

Consequence for the existing `GomocupEngine` against this engine (by reading, not by running):
- Move search works: `START`, `BOARD`, `DONE`, `x,y` match.
- Rules are not applied (`INFO RULE` ignored) and turn/depth/node limits are not applied: `INFO TIMEOUT_TURN` is
  uppercase and KataGomo compares `timeout_turn` in lowercase; `MAX_DEPTH`/`MAX_NODE` are unknown to it.
- `stop()` cannot interrupt a search.

Open: whether Rapfi also accepts lowercase `timeout_turn` (GOMO-02 only used the uppercase form).

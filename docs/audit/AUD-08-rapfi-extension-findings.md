---
id: AUD-08-rapfi-extension-findings
type: audit
size: S
status: active
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
---
# Audit: Rapfi extension commands, their replies, and how a one-shot reply ends
Read-only audit of the Rapfi Gomocup front end (no code kept). A non-bug decision or review.
Immutable; supersede it with a new one.

## Context
Todo EXT-00 of planning `extensible-engine-commands` asks to check the planning's "Findings tu ma
Rapfi" table against the source, and to settle how a reply to a one-shot command ends (input for
the `Until(predicate)` completion mode of EXT-10). Sources read, both `Rapfi/command/gomocup.cpp`:

- **V2**: `Lab/Rapfi_V2/rapfi`, git commit `d14b636` (2026-11-11), file clean against that commit
  (`git status --porcelain` prints nothing). Copies under `build/` ignored.
- **V3**: `Lab/Rapfi_V3_PTR/dev/rapfi` (not a git repo). Identified by file mtime
  2026-10-27 00:56 and sha256 prefix `7084119f39ca` of `command/gomocup.cpp` (1260 lines). This is
  the tree the planning's V3 line numbers refer to: it has `case 8` (Caro) at :235 and the
  SWAP2BOARD Caro ERROR at :985. `Lab/Rapfi_V3_PTR/stable` is older: no Caro case, no rectangular
  board (`board->size()` instead of `width()/height()`), so it does not match.

Helpers: `MESSAGEL(x)` prints `MESSAGE x`, `ERRORL(x)` prints `ERROR x`, `DEBUGL(x)` prints
`DEBUG x` only in non-NDEBUG builds (`core/iohelper.h`). Line numbers drift: the enclosing function
is always named. "V3" below means the dev tree.

## Findings

### How commands are read (both versions)
`runProtocol` reads one whitespace-separated token with `std::cin >> cmd`, not a line. Each handler
then reads its own arguments from `std::cin`. Consequences used below:
- A handler that does not consume its arguments (unknown `INFO` key, unknown command) leaves them in
  the stream, and they are read later as commands (`ERROR Unknown command: <token>`).
- After `YXSHOWINFO` (GUIMode) the last branch of `runProtocol` prints nothing for an unknown
  command (V3 :1192, V2 :1049): an unknown command then produces no output at all.

### INFO VCTHREAD
- Line: `INFO VCTHREAD <n>`, n in 0..3.
- V2 (`getOption`, :271-290): 0 sets VCN off, no reply. 1, 2, 3 reply one line each:
  `MESSAGE VCN Mode enabled: VCT (Defender passes 2 times)`,
  `MESSAGE VCN Mode enabled: VC2 (Defender passes 3 times)`,
  `MESSAGE VCN Mode enabled: VC4 (Defender passes 1 time)`.
  Other values: `ERROR Unknown vcthread value: <n>, must be 0..3.` No search.
  V2 also has `YXVCN <2..4>` (`searchVCN`, :767-782): one search at that VCN level, then restores
  the VCTHREAD level; bad level gives `ERROR YXVCN level must be 2..4.` and no search.
- V3 (`getOption`, :321-324): VCTHREAD is a protocol no-op. The value is read and ignored, no
  reply. `YXVCN` does not exist in V3.

### YXANALZ
- Absent in V3 (see the consequences under "How commands are read").
- V2 (`analyzeMoves`, doc :705, body :713-760; dispatch in `runProtocol` :1029 via
  `CheckBoardOK`). Lines: `YXANALZ`, then `x,y` tokens, then `DONE` (EOF also ends the list).
- Each token is checked with `parseLegalCoord`; per-entry rejections print one ERROR and skip the
  entry: `ERROR Coord is not valid or empty.`, `ERROR Analyze move can not be a pass, ignored.`,
  `ERROR Analyze move is a forbidden point, ignored.` (Renju, black to move). Duplicates are
  dropped silently.
- If no entry survives: `ERROR No valid analyze move` and **no search** (no move line follows).
- Otherwise it starts a real search (`think`) with `multiPV` = list size and the list as the root
  whitelist: per-PV output while searching, then the normal final move line. It is a search, not
  a one-shot command.

### INFO RULE 8 (Caro)
- Line: `INFO RULE 8`. No reply on success.
- V3 (`getOption`, `case 8` at :235): Caro with free opening. `SWAP2BOARD` under Caro
  (`swap2board`, :979; ERROR at :985) first reads and applies the block (`getPosition(false)`), then
  prints `ERROR SWAP2BOARD: the Caro rule has no swap opening (free opening only).` and starts no
  search.
- V3 also refuses `INFO RULE 2`/`4` (Renju) on a torus: `ERROR INFO RULE: the Renju rule is not
  supported on a torus board (INFO TORUS 1); rule unchanged.` (:220-225).
- V2 (`getOption`, `RULE` branch :232-266): id 8 hits `default`:
  `ERROR Unknown rule id: 8. Rule is reset to freestyle...` and the rule becomes freestyle.

### INFO WALL
- Line: `INFO WALL x,y`. No reply on success.
- V3 (`getOption`, `WALL` branch :449-494). Rejections, each one ERROR line, nothing changes:
  malformed (`ERROR INFO WALL coord is malformed, expected "INFO WALL x,y".`, stream recovered),
  before START (`ERROR INFO WALL requires START first.`), off board or pass
  (`ERROR Wall coord is a pass or invalid.`), on a portal cell (`ERROR Wall coord is a portal
  cell.`). On success it calls `newGame` and clears the TT.
- Walls are cleared by `START`/`RECTSTART` (`startBoard`, :679-694), by `INFO CLEARPORTALS`
  (:585-600, clears walls and portals, keeps torus), and by every `BOARD`/`YXBOARD`/`SWAP2BOARD`
  block (`getPosition`, :765-864): the block's `c=3` entries replace the wall set, so a block with
  no `c=3` clears all walls. `RESTART` (`restart`, :671) does **not** clear walls.
- V2: no WALL key. `INFO WALL 7,7` prints `MESSAGE Unknown Info Parameter: WALL` (`getOption`,
  :425), then `7,7` is read as a command: `ERROR Unknown command: 7,7` (silent in GUIMode). Block
  entries with `c=3` are parsed and skipped (`getPosition`, :615 "Currently wall is not
  supported").

### INFO TORUS
- Line: `INFO TORUS 0|1`; one value wraps both axes. No reply on success.
- V3 (`getOption`, `TORUS` branch :535-584). Rejections, each one ERROR line, nothing changes:
  malformed, value not 0/1, before START, `INFO TORUS 1` under Renju, board smaller than
  `MIN_TORUS_BOARD_SIZE`, portals that would be closer than `MIN_PORTAL_DISTANCE` across the edge,
  rectangular board (`ERROR INFO TORUS 1 is not supported on a rectangular board.`).
- Reset to off by `START`/`RECTSTART` (`startBoard`). Not reset by `INFO CLEARPORTALS`, `RESTART`
  or a `BOARD` block.
- V2: unknown key, same leftover-argument effect as INFO WALL.

### INFO YXPORTAL
- Line: `INFO YXPORTAL x,y x,y` (one pair of cells). No reply on success.
- V3 (`getOption`, `YXPORTAL` branch :495-534). Rejections, each one ERROR line: malformed, before
  START, off board or pass, on a wall cell, cells closer than `MIN_PORTAL_DISTANCE` (Chebyshev),
  rectangular board (`ERROR INFO YXPORTAL is not supported on a rectangular board.`).
- Cleared by `START`/`RECTSTART` and `INFO CLEARPORTALS`; a `BOARD` block does not clear portals.
- V2: unknown key, same leftover-argument effect as INFO WALL.

### RECTSTART
- Line: `RECTSTART w,h` (protocol width and height).
- V3 (`rectStart`, :708-729; shared `startBoard`, :679-694). Success: one line `OK` (printed by
  `restart`). Under coordinate mode `FLIPY_X` the internal board is h x w. Rejections:
  `ERROR Malformed RECTSTART, expected RECTSTART <w>,<h>.` (fail state cleared, rest of the line
  not discarded), `ERROR Unsupported board size!` (w or h outside 5..MAX_BOARD_SIZE).
- V2 (`rectStart`, :517-523): present but always refuses: reads `w,h`, prints
  `ERROR Rectangular board is not supported.`, no `OK`, board unchanged.

### BOARD, YXBOARD, SWAP2BOARD, TURN, YXBLOCK
All need a started game: otherwise `ERROR No game has been started.` (`CheckBoardOK` in
`runProtocol`) and nothing else.
- **Block format** (`getPosition`; V3 :765-864, V2 :566-653): after the command, tokens `x,y,c`
  until `DONE` (or EOF). `c`: 1 = the engine's own stone, 2 = the opponent's, 3 = wall (V3 only).
  The first non-wall entry decides who is black; a PASS is inserted when two entries of the same
  side follow each other. Per-entry error: `ERROR Coord is not valid or empty.` or
  `ERROR Color is not a valid value, must be one of [1, 2, 3].`, entry skipped. Fatal for the
  command (position left half built, no search): consecutive passes
  (`ERROR Consecutive pass is not supported. ...`); V2 also `ERROR Too many moves in the
  position.` and duplicates (`ERROR Coord is not valid or empty.`).
- `BOARD` (V3 :1166, V2 :1023): block, then starts a search; reply is the search output and the
  final move line.
- `YXBOARD`: block only, no search, **no reply** on success.
- `SWAP2BOARD` (`swap2board`; V3 :979-1015, V2 :825-856): sets the opening rule to SWAP2 (it stays
  SWAP2 afterwards), reads the block. Empty board: one line with three coordinates
  (`x,y x,y x,y`, possibly preceded by `MESSAGE No available opening is in database!`), no search.
  Otherwise a search with swap allowed; the final line is a move, `SWAP`, or two moves
  (`x,y x,y`). In the two-move case (`Swap2PutTwo` in `sendActionAndUpdateBoard`) a second search
  runs from the callback, and V3 leaves `thinking` false during it.
- `TURN x,y` (`turn`; V3 :753-763, V2 :551-564): one line, not a block. Plays the opponent move
  and starts a search. Bad coordinate: `ERROR Coord is not valid or empty.` and no search.
- `YXBLOCK` (`getBlock`; V3 :866-899, V2 :655-695): tokens `x,y` until `DONE`, no colour, no
  search, no reply. An invalid coordinate prints `ERROR Block coord is a pass or invalid.` but is
  still added (no `continue`). The list survives `START`; only `YXBLOCKRESET` (all) or
  `YXBLOCKUNDO` (listed cells) remove entries.

### Which ERROR ends a search request
For the "what ends a search" question of ADR-01: some ERRORs mean no search will start (the
caller must not wait for a move), others are per-entry and the search still starts.

| Request | ERROR means no search (terminal) | ERROR, search still starts |
|---|---|---|
| any | `No game has been started.` | - |
| `BEGIN` | `Board is not empty.` | - |
| `TURN` | `Coord is not valid or empty.` | - |
| `BOARD` | consecutive pass; V2: too many moves, duplicate | bad coord or colour of one entry |
| `SWAP2BOARD` | V3 Caro refusal; same as BOARD | same as BOARD |
| `YXANALZ` (V2) | `No valid analyze move` | bad, pass or forbidden entry |
| `YXANALZ` (V3) | `Unknown command: YXANALZ` (nothing in GUIMode) | - |

### STOP, YXSTOP and commands dropped while thinking
- V3 (`runProtocol` :1127-1128): `STOP`/`YXSTOP` call `stopThinking()` and are read even while
  thinking. The running search then ends through its normal callback, which prints the best move
  so far (the normal terminal line). With nothing running, or during pondering, no output.
- V3 :1128 `else if (thinking) return false;`: every other command is dropped silently while
  `thinking` is true. Only the command token is consumed; its arguments stay in the stream and
  are read one token at a time, each also dropped while thinking. If the search ends partway
  through, the remaining tokens (for example `x,y,c` lines or `DONE` of a block) run as commands.
- V3 race (multi-threaded build): the `think` callback (:102-145) prints the move under
  `protocolMutex` and only afterwards sets `thinking = false`, while `runProtocol` tests
  `thinking` before taking the mutex. A command sent right after the move line can still be
  dropped.
- V2 (`runProtocol` :968-985): STOP calls `stopThinking()`, takes `protocolMutex`, and stops again
  (covers a ponder search started by the callback). The drop branch discards the rest of the line
  (`std::cin.ignore(..., '\n')`). The `think` callback (:99-161) clears `thinking` before printing
  the move (CMD-01), so the race above is fixed in V2.
- `thinking` is false during pondering, so commands sent while pondering are executed (most of them
  stop the ponder first; `TURN`, `YXSHOWFORBID`, `YXBLOCKRESET` and the `YXQUERYDATABASE*` family
  are exempt).

### ABOUT, YXSHOWINFO and how a one-shot reply ends
- `ABOUT` (`runProtocol`, V3 :1143, V2 :1000): exactly one line, no prefix, from `getEngineInfo`
  (`core/version.cpp`):
  `name="Rapfi", version="<ver> (<build>)", author="Rapfi developers (see AUTHORS file)",
  country="China"` (one line in the output).
- `YXSHOWINFO` (`setGUIMode`, V3 :147-155, V2 :163-171): exactly three lines,
  `MESSAGE Rapfi <ver> (<build>)`, `MESSAGE INFO MAX_THREAD_NUM 256`,
  `MESSAGE INFO MAX_HASH_SIZE 30`. Side effects: GUIMode on (unknown commands become silent) and
  message mode BRIEF becomes NORMAL (changes the MESSAGE lines a search prints).
- Other one-shot replies: `START`, `RECTSTART`, `RESTART`, `TAKEBACK` print `OK` or one `ERROR`;
  `INFO ...`, `YXBOARD`, `YXBLOCK`, `YXBLOCKRESET` print nothing on success (an ERROR or MESSAGE
  only on failure or for some values).

**Conclusion for `Until`:** Rapfi has no terminator line for one-shot replies. A reply is one of
(a) a fixed number of lines (ABOUT: 1 line starting `name="`; YXSHOWINFO: 3 `MESSAGE` lines, the
last containing `MAX_HASH_SIZE`), (b) `OK` or a single `ERROR` line, or (c) nothing on success.
So `Until` must take a per-command predicate (line count or a match on the last expected line, with
`ERROR` as an alternative end), and a command of kind (c) cannot be completed by `Until` at all; it
needs fire-and-forget or a timeout. A fence (sending `ABOUT` after a silent command and waiting for
its `name="` line, which works because commands are handled in order on one thread) is an option
for EXT-10 to weigh, not a decision here: `ABOUT` is itself dropped while thinking and stops a
ponder search.

## Corrections to the planning
The planning is frozen; these notes correct its Findings table for later todos.
- **VC2 / INFO VCTHREAD**: the `MESSAGE VCN Mode enabled: ...` reply and the 0..3 semantics are V2
  only. In V3 dev `INFO VCTHREAD` is read and ignored, with no reply.
- **YXANALZ**: V2 only; V3 has no such command (unknown command, and its tokens become unknown
  commands too). In V2 an empty or all-invalid list ends with `ERROR No valid analyze move` and no
  search, so a caller marked Busy must treat that ERROR as terminal.
- **Caro**: V2 does not know id 8 and silently falls back to freestyle after an ERROR line.
  `SWAP2BOARD` under Caro still applies the block before its ERROR. Line :981 is :985 in V3 dev
  (function `swap2board`, :979).
- **WALL**: `RESTART` keeps walls; `INFO CLEARPORTALS` also clears them. A block replaces walls;
  portals and torus are not carried by the block and survive it.
- **Torus**: add two more refusals: before START, and on a rectangular board.
- **RECTSTART**: exists in V2 but always refuses with an ERROR.
- **BOARD family**: `c=1` is the engine's own stone (the engine is to move after the block), not a
  fixed colour. `SWAP2BOARD` leaves the opening rule set to SWAP2. `YXBLOCK` accepts an invalid
  cell after printing ERROR, and its list survives START.
- **Stop**: "wait for the terminal event, then send" is sufficient on V2, but V3 dev has a race:
  `thinking` is cleared after the move is printed, so a command sent at once can still be dropped.
  V3 also leaves a dropped command's arguments in the stream, where they later run as commands.
- Line ranges otherwise drift by a few lines (WALL :449-494, YXPORTAL :495-534, TORUS :535-584,
  `rectStart` :708-729, block family :753-899 in V3 dev); the named functions are the reference.

## Status
active

## Supersedes
none

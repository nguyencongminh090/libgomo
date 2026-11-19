---
id: engine-protocol-port
type: planning
size: L
status: approved
owner: claude
created: 2026-11-19
supersedes: []
links: []
---
# Engine protocol port: one interface over Gomocup_extend and GTP

Discussion for an L-size feature. Approved by the owner on 2026-11-19; now frozen. Split todos
(each at most M) carry `planning: engine-protocol-port`.

## Goal

An application talks to any Gomoku engine through one protocol-neutral interface and does not
know which wire protocol is behind it. Two protocols are in scope for the first version:

- **Gomocup_extend**: Gomocup/piskvork plus Yixin-Board `YX*` extensions. Reference engine:
  Rapfi (`Rapfi/command/gomocup.cpp`).
- **GTP**: Go Text Protocol as adapted for Gomoku. Reference engine: KataGomo branch `Gom2026`
  (`cpp/command/gtp.cpp`).

The interface covers: start/stop an engine process, set options, set up a position, request a
move, cancel a search, and receive structured search info (depth, eval, nodes, PV, best move)
parsed from the engine's text output. Any board size the protocol allows (square or
rectangular).

## Non-goals

- No GUI, no game-playing loop, no tournament manager.
- No engine implementation and no vendoring of engine code.
- Engine-specific features with no cross-protocol meaning (Rapfi database commands, KataGo
  `kata-*` training commands) are reachable only through the raw escape hatch, never through the
  core interface.
- Other protocols (UCI-like, Piskvork over sockets) are not built, but must be addable without
  editing existing adapters.

## Constraints

- Low coupling and open extension (CLAUDE.md focus): a new protocol is a new adapter, with no
  change to the core or to existing adapters (OCP, DIP).
- The core depends on no protocol, process or I/O type. Process/pipe handling sits behind its
  own port so parsers can be tested on recorded text without spawning anything.
- C++23. No threads, coroutines or async framework in the core.
- Parsing is a separate, pure step: text line in, typed event out. Unknown lines become an
  `Unparsed` event and are never dropped silently.
- Evidence comes from real transcripts captured from Rapfi and KataGomo, used as test fixtures.
  Reference repos stay outside this repo and are treated as untrusted input.

## Options

Facts that drive the choice (from the two reference engines):

| Aspect | Gomocup_extend (Rapfi) | GTP (KataGomo) |
|---|---|---|
| Position model | Sent as a full list per `BOARD`/`TURN`; engine can be near-stateless | Stateful: `play`/`undo`/`clear_board`/`set_position` mutate the engine board |
| Reply framing | Free-form lines: coordinates, `MESSAGE`, `DEBUG`, `UNKNOWN`, `ERROR`, `SWAP` | `=`/`?` prefix, id optional, blank-line terminator |
| Move syntax | `x,y` decimal | letter+number vertex, plus `pass` |
| Search info | `MESSAGE`/`INFO`-style lines, engine-specific | `lz-`/`kata-analyze` streams |
| Options | `INFO key value` before search | `kata-set-param`, `boardsize`, `time_settings`, ... |
| Termination | `END` | `quit` |

- **A. Lowest-common-denominator command set** (`newGame`, `play`, `genMove`, `stop`) with
  the adapter mapping each. Simple, but hides the stateless/stateful difference and loses
  structured search info.
- **B. Ports and adapters with a neutral position model.** The core owns a `Position`
  (size + ordered move list + side to move). `EnginePort::search(Position, Limits)` is the one
  call. The GTP adapter diffs against the engine's last known position and issues
  `play`/`undo`/`clear_board`; the Gomocup adapter sends `BOARD`/`TURN`. Output flows through a
  per-protocol `LineParser` into a neutral `SearchEvent` stream. Pattern: Adapter (+ Strategy
  for parsing). Notes: `style-hexagonal`, `pattern-adapter`.
- **C. Per-protocol APIs with a thin common header.** Least abstraction, but pushes the
  difference onto every caller and defeats the project's purpose.

## Decision

**B**. The position-diffing cost in the GTP adapter is the price of keeping callers
protocol-blind, and it is contained in one adapter.

- **Concurrency:** non-blocking `startSearch(position, limits, sink)` plus `stop()`; events
  (info, best move, error, `Unparsed`) are pushed to an `EventSink`. A blocking `search()`
  helper is built on top. Threads live only inside the transport adapter.
- **Parsing:** hand-written line parsers on `std::string_view`, `std::from_chars` and
  `std::expected` (the grammars are line/token based, not nested; a parser library adds a
  dependency and compile cost for little gain; `std::regex` is avoided). A narrow library (e.g.
  CTRE) may be added inside one adapter later without changing the core.
- **Process:** a `Transport` port (write line, receive line/close event) with two
  implementations: `ProcessTransport` (spawns the engine and owns the pipes; POSIX first) and
  `StreamTransport` (takes an already connected stream; used by tests and sockets).
- **Build:** CMake.
- **Coordinates:** the core uses 0-based (x, y) with the origin at the top-left; each adapter
  converts. GTP vertices (from `KataGomo/cpp/game/board.cpp`, `Location::toString`): column
  letters `A..Z` skipping `I`; column 25 and up use two letters; the row number counts from
  the bottom (`row = ySize - y`); boards beyond 625 columns fall back to a machine format. The
  adapter converts all of this and rejects what it cannot encode.
- **Pass and swap:** `Swap` is a first-class event, reported only where the engine speaks it
  (Rapfi). GTP `pass` maps to a `Pass` event that callers may treat as an error in Gomoku. For
  GTP the caller (referee/app) runs a swap opening, because KataGomo has none.
- **Escape hatch:** v1 includes `sendRaw(line)` and raw reply events, with no parsing
  guarantees. Engine-only fields (VCNRule, `firstPassWin`, `DRAW_RESULT`) go through it.
- **Rules:** core `Rules = {BoardRule{Freestyle,Standard,Renju},
  OpeningRule{FreeOpen,Swap1,Swap2}, optional maxMoves}`. Mapping:

  | Core | Gomocup_extend (Rapfi) | GTP (KataGomo) |
  |---|---|---|
  | `{Freestyle, FreeOpen}` | `INFO RULE 0` | `kata-set-rule basicrule FREESTYLE` |
  | `{Standard, FreeOpen}` | `INFO RULE 1` | `kata-set-rule basicrule STANDARD` |
  | `{Renju, FreeOpen}` | `INFO RULE 4` | `kata-set-rule basicrule RENJU` |
  | `{Freestyle, Swap1/Swap2}` | `INFO RULE 5` / `6` | unsupported |
  | `{Standard/Renju, Swap*}` | unsupported | unsupported |
  | `maxMoves` | `INFO MAX_MOVES n` | `maxmoves n` |

  Unsupported combinations return `std::unexpected(Unsupported)` and send nothing (Rapfi would
  silently reset an unknown id to freestyle). Each adapter exposes `supports(Rules)`. The GTP
  adapter verifies the setting with `kata-get-rules`. The Gomocup adapter always sends id `4`
  for Renju and accepts `2` only as a Yixin-Board alias when reading.

## Open questions

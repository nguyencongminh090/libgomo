---
id: ADR-02-extensible-engine-commands
type: adr
size: L
status: active
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands, ADR-01-info-event-candidates, AUD-08-rapfi-extension-findings, EXT-01-is-terminal-helper, EXT-02-value-and-custom-event, EXT-03-line-parser-gomocup, EXT-04-line-parser-gtp-katagomo, EXT-05-rules-extensions, EXT-06-board-layout-position, EXT-07-request-encoder-gomocup, EXT-08-request-encoder-gtp-katagomo, EXT-09-rect-boards, EXT-10-command-port-force]
---
# The core and adapters open five extension points for engine-specific commands
A hard-to-reverse architecture decision. Immutable; supersede it with a new one.

## Context
An application with its own engine (for example Rapfi V2/V3 with `YXANALZ`, Caro, WALL, Torus,
Portal) could not use PyGomo without editing it. Five places were closed (planning
`features/extensible-engine-commands/planning.md`, read from `include/` on 2026-11-23):

1. Commands out: only `EnginePort::sendRaw(line)`. No typed reply, no link between a reply and the
   command that caused it.
2. Parsing: `parseGomocupLine` was called directly by a `final` adapter. A new output shape meant
   a new adapter.
3. Rules: two closed enums. Caro had no representation, so `supports()` said `false`.
4. Events: `SearchEvent` is a closed `std::variant`. Unknown output was `MessageEvent` or
   `UnparsedEvent`, raw text the application had to parse itself.
5. Board structure: `Position` knew width, height and moves. A WALL cell, a wrapping edge or a
   portal pair had no place.

Constraints that shaped the decision:

- The core stays protocol-neutral and includes no I/O, thread, process or protocol header
  (`scripts/check-core-deps.sh`). New core types are plain data.
- No break of the public API: existing code compiles and behaves the same (new members last,
  new parameters defaulted, new enumerators last).
- ADR-01: four places treated "every type other than Info, Message, Unparsed" as the end of a
  search. A new `SearchEvent` type would have ended searches at its first line.
- Rapfi silently drops every command except `STOP`/`YXSTOP` while it thinks (AUD-08). A new
  command must never be written before the running search has ended.
- Rapfi's behaviour differs by version and the library cannot see which one it talks to
  (AUD-08, "Corrections to the planning").

The owner chose the options below on 2026-11-23 and approved the planning. Implementation is
EXT-01 to EXT-10.

## Decision
Eight additions, all opt-in. An application that sets none of them gets the old behaviour.

**1. `SearchEvent` gains `CustomEvent`, and `isTerminal` is the one terminal test.**
`CustomEvent{tag, fields}` is appended to the variant; `fields` is an ordered list of
key and `Value` pairs. It is not terminal. `isTerminal(const SearchEvent&)` in
`core/search_event.hpp` replaced the four copies of the condition (`search()`, `DeadlineEngine`,
`KataGomoEngine`, `GomocupEngine`); every adapter and decorator now calls it, so a later event
type is classified in one place.

**2. `Value` is a closed, typed variant.**
`std::variant<bool, std::int64_t, double, std::string, Coord, Move, std::vector<Coord>,
std::vector<Move>>` with a default `format(Value)`. It is the argument type of commands, the
field type of `CustomEvent`, and the value type of `Rules::extensions` and
`BoardLayout::extensions`. A shape outside the set is built as a line directly by the engine's
encoder, not through `Value`.

**3. `LineParser` and `chainParsers` (namespace `pygomo::parse`).**
`LineParser = std::function<SearchEvent(std::string_view)>`. `GomocupEngine`, `GtpEngine` and
`KataGomoEngine` take one in their constructor; empty means the built-in parser.
`chainParsers(custom, fallback)` asks `custom` first and falls back when it answers
`UnparsedEvent`. The parser runs on the reader thread, outside the adapter lock, so it must not
block, and it must not turn a line that ends a search (a move, `SWAP`, a fatal `ERROR`) into a
non-terminal event: the adapter trusts `isTerminal` of the result.
GTP rule: the custom parser sees each non-empty reply body line and free-text line of the running
search, first. A non-terminal, non-Unparsed result goes to the sink and consumes the line;
Unparsed or terminal falls back to the built-in handling. Only a reply ends a GTP search, so a
parser that consumes the final `play X` of an analyze reply loses the move. Reply headers are
never shown to it.

**4. `Rules::extensions`.**
`std::map<std::string, Value>` at the end of `Rules`, for rules the enums do not model (Caro). The
default adapter refuses a non-empty map: `supports()` is `false` and `setRules` returns
`Unsupported`. `BoardRule` and `OpeningRule` are unchanged.

**5. `BoardLayout` in `Position`, and `PositionError::Blocked`.**
`BoardLayout{torus, blocked, portals, extensions}` is data about the board, not a rule.
`Position::create(w, h, layout = {})` returns `OutOfBounds` for a blocked cell or a portal end off
the board. `play()` returns `Blocked` for a WALL cell (check order: OutOfBounds, Blocked,
Occupied). `moves()` and `sideToMove()` do not change: a wall is not a move. `operator==` compares
the layout. The core checks nothing the engine checks (torus size, portals on a rectangle, Renju
on a torus): the engine answers `ERROR`. `Blocked` was added last in the enum; no `switch` on
`PositionError` existed in the repository.

**6. `SearchRequest`, `RequestEncoder` and `pygomo::encode`.**
`SearchRequest{rules, position, limits}` is everything one search asks for.
`RequestEncoder = std::function<EncodedRequest(const SearchRequest&)>`, with
`EncodedRequest = std::expected<std::vector<std::string>, EngineError>`, replaces the whole send
script of one `startSearch`. Each adapter takes one in its constructor; empty means its default.
The defaults are public so an application edits instead of rewriting:
`encode::defaultGomocupRequest`, `encode::defaultGtpRequest`, `encode::defaultKataGomoRequest`,
and `encode::boardBlock(command, moves, extra)` for the `x,y,c` block. An error from the encoder
(normally `Unsupported`) means nothing is sent. The encoder runs under the adapter lock: it must
be pure, must not block and must not call back into the adapter.
Consequence for `supports()`: without an encoder, valid rules with empty `extensions` and an
expressible rule id; with an encoder, any rules that pass `validate()`, because only the encoder
knows what it can send. It then returns `Unsupported` from `startSearch`, not from `supports`.
The defaults refuse `Rules::extensions` and a non-default `BoardLayout` with `Unsupported`; they
never drop them silently.

**7. Rectangular boards.**
`GomocupEngine`'s default script sends `RECTSTART w,h` for a non-square board instead of
`START n`. `GtpEngine` already sent `boardsize w h`. `KataGomoEngine` stays `Unsupported` for
non-square boards: `gom` mode only has `START n` (AUD-02, from the source).

**8. `CommandPort`, a second interface beside `EnginePort`.**
`CommandPort::execute(const Command&, EventSink&, CommandOptions)` and
`canExecute(const Command&)`. `EnginePort` is unchanged. `Command` is data: `name`, `lines`,
`args`, `block` (`Value`s, one line each), `terminator` (for example `DONE`) and a `completion`:

- `None`: done once the lines are written; the sink gets no event.
- `Search`: ends at the terminal event (`isTerminal`), Busy until then, like `startSearch`.
- `Until{done}`: ends at the first event for which `done` is true; that event still reaches the
  sink.

`CommandOptions::force` stops the running work (`stop()`, `YXSTOP` for Gomocup), queues the
command, and writes it from the reader thread after the old terminal event has reached the old
sink, so no output is lost and nothing is written while the engine is thinking. One queued
command at most; a second `force` returns `Busy`. `GomocupEngine`, `KataGomoEngine` and
`DeadlineEngine` implement the interface; `DeadlineEngine` forwards to the wrapped engine when it
is a `CommandPort`.
Decisions made during implementation (EXT-10 Evidence):

- The wire form of a command is the public `commandLines(command)`, so a test and an adapter
  agree on it.
- `GtpEngine` supports `None` only, one line per command: a GTP command has one reply and the
  reply to an arbitrary command is no move for a known position. `canExecute` says so. With
  `force` during a search it waits for the natural end (`stop()` cannot interrupt `genmove`).
- A command has no deadline, because it has no `Limits`. Bound a `Search` command with `stop()` or
  use `Until`. If the search a `force` command waits for times out in `DeadlineEngine`, the
  command is never sent (the engine may still be thinking) and its sink gets an `ErrorEvent`.
- `GomocupEngine` now serializes every write (script, command, `YXSTOP`, `sendRaw`) under one
  write lock, so `YXSTOP` cannot land inside another thread's `BOARD` block.
- A queued command that is dropped by a close, or whose write fails, gets one `ErrorEvent` on its
  own sink.

### Rejected options
Numbered as in the planning; the first option of each group is the decision above.

- Commands: add `execute` to `EnginePort` (every adapter, every test fake and `DeadlineEngine`
  would have to implement it); keep `sendRaw` only (today's state).
- Parser: drop `final` and let applications subclass (exposes `onLine` and the internal lock);
  make the parser a template parameter (moves each adapter into its header).
- Rules: `BoardRule::Custom` with a name (mixes two layers, every `switch` needs the new branch);
  `Rules` as a `std::variant` (breaks every user).
- Events: carry the text in `MessageEvent` (no type, the application re-parses); a custom terminal
  event (what ends a search would be decided by an adapter, not the core; `Until` covers the need).
- Board structure: `Wrap {None, Horizontal, Vertical, Both}` (Rapfi has one switch for both axes,
  so `bool torus`); a wall as a stone in `moves()` (breaks `sideToMove()` and the requirement that
  moves do not change); walls in `Rules::extensions` (wrong layer, and `Position` would still
  accept a stone on a wall).
- Request building: a `SetupEncoder` that only adds lines after `START` (walls live in the
  `BOARD` block, `YXBOARD` does not start a search, `SWAP2BOARD` differs); one flag per variant in
  the adapter's `Options` (a new flag for every engine).
- `Value` as a string (loses type; the encoder would parse its own arguments).

## Consequences
- `CustomEvent`, `isTerminal`, `Value`, `LineParser`, `Rules::extensions`, `BoardLayout`,
  `PositionError::Blocked`, `SearchRequest`, `RequestEncoder`, the `pygomo::encode` builders and
  `CommandPort` are public interface. Changing them again is size L.
- Application code that holds a `switch` or `std::visit` over `SearchEvent` without a default
  branch now needs a `CustomEvent` case. `Rules::operator==` lost `constexpr` (it holds a
  `std::map`); no constant-expression caller existed.
- The default behaviour changed in three places the owner approved: a non-square board on
  Gomocup sends `RECTSTART` instead of being refused; a non-square board while Busy or closed
  now reports Busy or NotReady first (the check moved into the encoder); `GomocupEngine` writes
  are serialized. Two existing tests were changed for the first (`gomo_rect`; the refusal line in
  `ext07_default`).
- An application that sets an encoder owns the `Rules` and layout it accepts. The library no
  longer says `Unsupported` early for extensions it cannot see the meaning of.
- The library ships no Rapfi command. The fake-Rapfi example in the developer guide shows how an
  application writes them.
- The `tsan` preset does not build on master (a `-Werror` failure on g++ 15 that predates this work), so
  `ext10_*` was run with warnings-as-errors off only.

### Known gaps, accepted
The owner chose to build as planned and document the gaps found in AUD-08 ("Corrections to the
planning"). The code does not hide them:

- **Rapfi V3 can drop a command sent right after its move**: it clears `thinking` after printing the
  move, and it leaves a dropped command's arguments in its input, where they run later as
  commands. `force` waits for the terminal event, which is not enough on V3. `CommandOptions` and
  `GomocupEngine::execute` say so in their doc comments.
- **V2 and V3 differ.** `YXANALZ` exists only in V2 (and a list with no valid move ends with
  `ERROR No valid analyze move` and no search: a `Search` command then never completes, use
  `Until`). `INFO VCTHREAD` has a reply only in V2. `INFO RULE 8` (Caro) is an `ERROR` plus a
  fallback to freestyle in V2. `RECTSTART` always refuses in V2. The application picks the
  commands for its engine version.
- **`Until` can wait forever** for a one-shot command: Rapfi has no terminator line, a reply is a
  fixed number of lines, one `OK` or `ERROR`, or nothing on success. A command whose success is
  silent cannot be completed with `Until`; use `None` or bound it with `DeadlineEngine`.
- **A position block replaces the walls** (a block without `c=3` entries clears them), and
  portals and torus survive it. The encoder must put walls in the block, not in `INFO WALL`.
- **The illegal-move check ignores walls**: a move on a blocked cell returned by the engine is not
  rejected (EXT-07 Evidence).
- **`RECTSTART` and coordinates**: Rapfi's `FLIPY_X` coordinate mode swaps w and h, and no fixture
  covers a `coord_conversion_mode` other than `none`. The default encoder does not handle it.
- **`BoardLayout ==` is order-sensitive** (walls in another order compare unequal) and duplicate
  walls are accepted (EXT-06 Evidence).
- Not done: no adapter-neutral way to learn the engine version, no `Until` helper for a fixed
  count of lines, and `GtpEngine` supports no `Search` or `Until` command. Each needs its own todo.
- Checks: ctest `ext01_*` to `ext10_*` (the live `ext09_live` is skipped without
  `PYGOMO_RAPFI_V3`).

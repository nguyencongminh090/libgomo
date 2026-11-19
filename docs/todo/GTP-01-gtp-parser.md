---
id: GTP-01-gtp-parser
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# GTP response and vertex parser (source-derived)

## Scope
- In `pygomo_parse` (links `pygomo_core` only; pure, total, no streams, threads or transport):
  - `parseGtpResponseHeader(line)`: `=`/`?`, optional numeric id, rest of line (first line of a response).
    Body lines and the terminating blank line are the adapter's concern (GTP-02), not this parser.
  - `parseGtpVertex(text, boardSize)` -> `pass`, `resign`, or a `Coord` (0-based, same origin as `Coord`);
    `formatGtpVertex(Coord, boardSize)` for `play`/`set_position` arguments.
  - `parseGtpAnalyzeLine(line)` -> `InfoEvent` for KataGomo's `info move ... visits ... pv ...` lines
    (`kata-analyze`, `lz-analyze`): the first info entry gives depth-less `InfoEvent` with visits as nodes
    and the PV as `Coord`s; lines it cannot read come back as `UnparsedEvent`.
- Line shapes come from reading the KataGomo `Gom2026` source only (commit `9a0e65c`, shallow clone kept
  outside the repo, nothing built or run): `cpp/command/gtp.cpp` (input loop, `play`, `genmove`,
  `boardsize`, analyze callbacks), `cpp/game/board.cpp` (`Location::toString`, `Location::tryOfString`),
  `cpp/core/commandloop.cpp`.
- Fixtures in `tests/fixtures/gtp/` are written by hand from that source. Each has a header naming the
  source `file:line` it mirrors and the word `source-derived` (never `captured`).

## Don't
- No engine, no process, no live test: there is no GTP engine to run. Do not claim live coverage.
- Do not touch `pygomo_core` (message events and rule-set mapping are separate decisions).
- No adapter here (that is GTP-02); no board-size state in the parser beyond the argument.

## Traps (from the source)
- Letters skip `I`: `A..H`, `J..Z`. Row is counted from the bottom (`y_size - y`), so `A1` is bottom-left.
  Two-letter columns exist for x >= 25 (`AA`..); the machine form `(x,y)` appears when the board is huge
  or the loc is out of range. Input accepts lower case; `pass` and `pss` are both passes.
- Input side: `#` starts a comment; non-ASCII is dropped; tabs become spaces; a leading integer is the id.
  Reply is `=<id> body` or `?<id> body` (a space after `=` when there is no id), then a blank line.
- `genmove` may reply `resign` (only if the engine config allows it) and `pass`.
- `analyze` replies `=`, then streams info lines, then a blank line; any new input line stops it and emits a
  newline. `lz-analyze` prints winrate/prior/lcb as integers (x10000); `kata-analyze` prints floats.
  In Gom2026 `scoreMean`/`scoreLead` carry the draw rate, not a score; do not map them to `Score`.
- `set_position` rejects `pass` and overlapping stones; `play` on an illegal move answers `? illegal move`.

## Done-when
Acceptance lives in `docs/features.json` (`"todo": "GTP-01"`); `harness-track GTP-01` lists them.
(Features are proposed in the session and written only after the owner approves them.)

## Evidence
2026-11-19, g++ 15 default preset, warnings as errors: `gtp_header`, `gtp_vertex`, `gtp_analyze`, `gtp_total` pass; full suite 35/35 (3 live Rapfi tests skipped, as before); `scripts/check-core-deps.sh` OK; tsan preset 35/35.
First run of `gtp_vertex` failed on two wrong expectations in my own test (a 15-column board ends at `P` because `I` is skipped); parser unchanged, test corrected.
Not covered: nothing here was run against a real GTP engine; all shapes are read from KataGomo source. `gtp_total` is a deterministic random plus extreme-length test; no ASan/UBSan run (tsan does not detect out-of-bounds reads).

## Links
- `docs/handoff.md`, `docs/todo/PARSE-01-gomocup-line-parser.md` (same parser shape).
- Source reference: KataGomo `Gom2026` @ `9a0e65c` (outside the repo).

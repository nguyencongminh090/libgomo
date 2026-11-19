---
id: GTP-02-gtp-adapter
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# GTP engine adapter (source-derived, fake-transport tested)

## Scope
- New library `pygomo_gtp` (links `pygomo_core`, `pygomo_parse`, `pygomo_transport`):
  `GtpEngine : EnginePort` over a `Transport`, same shape as `GomocupEngine` (`open()`, mutex, sink pointer).
- `startSearch`: sends `boardsize W H`, `clear_board`, the rule and limit commands, one `play <B|W> <vertex>`
  per move, then `genmove <B|W>`. Every command carries an increasing id; replies are matched by id.
  Reply framing: header line (`parseGtpResponseHeader`), body lines, blank line ends it.
- Rules (from KataGomo `cpp/command/gtp.cpp`, `cpp/game/rules.cpp`): `FREESTYLE`, `STANDARD`, `RENJU` are
  commands taking no argument; `maxmoves N`. Only `FreeOpen` opening is expressible; `Swap1`/`Swap2` are
  `Unsupported` and nothing is sent.
- Limits: `turnTime` -> `kata-set-param maxTime <seconds>`, `maxNodes` -> `kata-set-param maxVisits <n>`.
  `matchTime` and `maxDepth` have no equivalent: `Unsupported`, nothing sent.
- Result of `genmove`: vertex -> `BestMoveEvent` (checked against the position, as in GomocupEngine), `pass`
  -> `PassEvent`, `resign` -> `ErrorEvent` ("engine resigned"; core has no resign event, owner's decision),
  `?` reply to any command -> one terminal `ErrorEvent` holding the engine's text.
- Tests use a scripted fake `Transport` replaying shapes read from the KataGomo source. No live test.

## Don't
- No `kata-genmove_analyze` / `kata-analyze` streaming here (a later todo); `genmove` only.
- Do not touch `pygomo_core` or the parsers. No process or engine code in this repo.
- Do not claim the adapter works against a real engine.

## Traps (from the source)
- KataGomo reads stdin in one thread: `genmove` runs inside the read loop, so a `stop` line sent during
  `genmove` is only read after the move is already chosen. `stop()` therefore cannot interrupt a search; it
  must not pretend to. Searches are bounded by the limits only. (`stop` works only for analyze commands.)
- Setup commands are pipelined; one `?` reply means the rest of the replies still arrive. Count them and end
  the search once, on the first failure, then ignore the remaining replies of that search.
- `play` on a stone the engine thinks illegal answers `? illegal move`; `boardsize` rejects sizes < 2.
- The engine may print log text on stderr only; stdout lines outside a reply are dropped like in GomocupEngine.

## Done-when
Acceptance lives in `docs/features.json` (`"todo": "GTP-02"`); `harness-track GTP-02` lists them.
(Features are proposed in the session and written only after the owner approves them.)

## Evidence
2026-11-19, g++ 15 default preset, warnings as errors: `gtp2_sequence`, `gtp2_result`, `gtp2_rules`, `gtp2_lifecycle` pass; full suite 39/39 (3 live Rapfi tests skipped); tsan preset 39/39; `scripts/check-core-deps.sh` OK.
First run of `gtp2_rules` failed on one wrong expectation in my own test (a 20-column board ends at `U` because `I` is skipped); adapter unchanged, test corrected.
Not covered: no real GTP engine was run; all reply shapes are from KataGomo source. KataGomo keeps `maxTime`/`maxVisits`/`maxmoves` between searches, so a later search without a limit inherits the earlier one (the adapter cannot reset to the engine default). `stop()` is a no-op by design.

## Links
- `docs/todo/GTP-01-gtp-parser.md`, `docs/todo/GOMO-01-gomocup-adapter.md`, `src/gomocup/gomocup_engine.cpp`.
- Source reference: KataGomo `Gom2026` @ `9a0e65c` (outside the repo).

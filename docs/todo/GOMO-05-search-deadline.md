---
id: GOMO-05-search-deadline
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# GomocupEngine: a search with a turn time has a deadline

## Scope
- When `Limits::turnTime` is set and > 0, a search gets a deadline of `turnTime + grace` (grace default 2000 ms, `GomocupEngine::Options::grace`, so tests can use a few ms).
- At the deadline the engine sends `YXSTOP`; after one more `grace` without a terminal event the search ends with `ErrorEvent{"engine did not answer"}` and the sink is released.
- One watchdog thread per engine, started in `open()`, joined in the destructor. No deadline without `turnTime`.
- Update the class comment in `gomocup_engine.hpp`.

## Don't
- Touch `EnginePort`, `GtpEngine` or `KataGomoEngine` (owner chose option 1: Gomocup only).
- Change what a normal search does: an engine that answers in time sees no difference.

## Traps
- The watchdog calls the sink: never under the lock, never after the sink was released (a move and the timeout race; whoever clears `sink_` under the lock owns the call).
- A sink may call `startSearch` from `onEvent` on the watchdog thread: the thread must never join itself (hence one long-lived thread, no per-search thread).
- A late move from the timed-out search arrives with no sink and is dropped; if a new search already started it is taken as that search's answer (only the legality check guards it). The engine is reported as not answering; the caller should close it.
- `YXSTOP` written by the watchdog can race with a new search start (same as `stop()`).

## Done-when
Acceptance in `docs/features.json` (`"todo": "GOMO-05"`).

## Links
Source: handoff "Uncertain" point on `GomocupEngine` (no timeout); owner decision 2026-11-20 (option 1).

## Evidence
`gomo_deadline` (4 cases) passes 20x in a row on default and tsan. `ctest --preset default` 55/55 and `--preset tsan` pass (live skipped, no real Rapfi run), `scripts/check-core-deps.sh` OK. Live: `PYGOMO_RAPFI=…/build/pbrain-rapfi ctest -R gomo2_` 5/5 x5 on default and once on tsan (engine answers within turnTime, so the deadline never fires; this only shows no regression with the real engine). The timeout path itself is tested with a fake transport only.

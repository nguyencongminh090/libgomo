---
id: DEAD-01-deadline-engine-decorator
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: engine-search-deadline
---
# DeadlineEngine: decorator that gives any EnginePort a search deadline

## Scope
- New `pygomo::engine` class `DeadlineEngine : EnginePort` wrapping an `EnginePort&` with `Options{grace=2000ms}`; `supports`, `setRules`, `sendRaw` forward.
- `startSearch` with `turnTime` > 0 arms a deadline of `turnTime + grace`: at it `inner.stop()`; after one more grace the sink gets `ErrorEvent{"engine did not answer"}` and the relay is detached. No `turnTime` (or 0) = no deadline.
- While detached `startSearch` returns `Busy` without calling the inner engine; the inner engine's late terminal event is dropped and clears it.
- Tests with a fake `EnginePort`: timeout, move in time, no turnTime, turnTime 0, sink calls `startSearch` from the timeout callback, late answer dropped then reusable, late non-terminal events dropped, inner start error leaves no state.

## Don't
- Change `GomocupEngine`, `GtpEngine`, `KataGomoEngine` or core.
- Write to the engine itself (only `inner.stop()`).

## Traps
- The sink is called from the inner engine's thread or the watchdog, never under the lock; a terminal event and the timeout race: whoever clears the sink pointer under the lock owns the call.
- Inner `startSearch` may emit events before it returns: install the sink before the call and undo on error.
- Library `pygomo::engine` must link `Threads` (it does) and may not be linked into core.

## Done-when
Acceptance in `docs/features.json` (`"todo": "DEAD-01"`).

## Links
Planning: `features/engine-search-deadline/planning.md` (approved 2026-11-20). Audit: `docs/audit/AUD-01-late-move-owed-answer.md`.

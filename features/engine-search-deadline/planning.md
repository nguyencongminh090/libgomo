---
id: engine-search-deadline
type: planning
size: L
status: draft
owner: claude
created: 2026-11-20
supersedes: []
links: [engine-protocol-port]
---
# Search deadline for every engine adapter
Discussion for an L-size feature, kept in `features/engine-search-deadline/planning.md`. Open questions must be empty to approve. After approval it is frozen; split todos (each at most M) carry `planning: engine-search-deadline`.

## Goal
A search that has a `Limits::turnTime` ends with a terminal event even when the engine never answers, for every adapter (Gomocup, GTP, KataGomo), with one behaviour and one place to maintain it.

## Non-goals
- No change to how an engine is asked to stop (`stop()` per adapter stays; GTP/KataGomo cannot interrupt, AUD-02).
- No recovery of a hung engine: the caller closes it.
- No deadline for a search without `turnTime` (or with 0).

## Constraints
- `pygomo_core` must stay I/O-, thread- and process-free (`scripts/check-core-deps.sh`), so a timer cannot live in `EnginePort` or in core.
- `GomocupEngine` already has a deadline (GOMO-05) and an owed-answer state (GOMO-06, `docs/audit/AUD-01`). GTP has request ids, so a late reply is told apart already (`gtp_engine.cpp:112`); KataGomo (`gom` mode) has no ids and Rapfi-like engines may drop commands while thinking, so no sync command can be used.
- The port rule "exactly one terminal event per search" and "sink never called under a lock" stay.
- Each todo at most M and the build stays green after each.

## Options
- A. Shared helper (a `SearchDeadline` class in its own library). Each adapter owns one and calls `arm()`/`disarm()` around its `sink_` changes, and supplies its own timeout action. Adapters keep their state machines; three places need the arm/disarm calls, and each adapter still needs its own owed-answer logic.
- B. Decorator `DeadlineEngine : EnginePort` that wraps any `EnginePort` (library `pygomo::engine`, next to `blocking_search`). It forwards everything, wraps the sink, and on the deadline calls `inner.stop()`, then after a second grace sends `ErrorEvent{"engine did not answer"}` itself and drops anything the inner engine still sends for that search. The inner adapter stays busy until its real answer comes, so a late move cannot reach a later search (the AUD-01 guarantee comes for free, as `Busy` from the inner engine). No adapter code changes for GTP and KataGomo.
- C. Put the timer into `EnginePort`. Rejected: it breaks the core dependency rule.

## Decision
Proposed (not approved): B, in todos of at most M each:
1. `DeadlineEngine` in `pygomo::engine` with a fake-port test (timeout, move in time, no turnTime, sink may call `startSearch` from the callback, late answer dropped and later search refused, tsan).
2. Use it with `GtpEngine` and `KataGomoEngine` fake-transport tests (timeout path, late reply, GTP ids).
3. Retire the built-in deadline and owed state from `GomocupEngine` in favour of the decorator; its `gomo_deadline` tests move to the decorator with the same cases. Needs the owner's answer to question 2.
Acceptance names `ctest --preset default`, `ctest --preset tsan` and `scripts/check-core-deps.sh`.

## Open questions
1. B (decorator) over A (shared helper)? Recommended: B, because it needs no change in the adapters and keeps the late-answer guarantee in one place.
2. Retire the built-in deadline of `GomocupEngine` (todo 3, behaviour moves to the decorator) or keep both? Keeping both means two timers per search and they would have to be given different graces so only one fires.
3. While an engine still owes an answer the caller sees `Busy` (from the inner engine) instead of today's `NotReady` from `GomocupEngine`. Acceptable?
4. Default grace 2000 ms, set at construction of the decorator: acceptable?

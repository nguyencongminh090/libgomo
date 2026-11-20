---
id: GOMO-06-late-move-after-timeout
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: [docs/audit/AUD-01-late-move-owed-answer.md]
planning: 
---
# GomocupEngine: a move that arrives after the timeout is not taken as the next search's answer

## Scope
- After GOMO-05 ends a search with "engine did not answer", the engine owes an answer: `startSearch` returns `EngineError::NotReady` until a move or swap line arrives (it is dropped, never sent to a sink) or the connection closes.
- Update the class comment in `gomocup_engine.hpp` (replaces the "taken as its answer" warning).
- Update the GOMO-05 tests that start a new search after a timeout.

## Don't
- Send any command to find out whether the engine is alive (see the audit: Rapfi drops commands while thinking).
- Add a way to clear the state by hand or a retry timer: a caller that wants a fresh engine closes it and opens a new one.
- Touch `GtpEngine`, `KataGomoEngine` or `EnginePort`.

## Traps
- The owed state is cleared under the same lock as `sink_`, so a late move and `startSearch` cannot both win.
- An `ERROR` or info line while owed does not clear it; only a parsed move or swap does.

## Done-when
Acceptance in `docs/features.json` (`"todo": "GOMO-06"`).

## Links
Source: handoff uncertain point from GOMO-05; owner choice 2026-11-20 (option 3).

## Evidence
`gomo_deadline` (5 cases) passes 20x in a row on default and tsan; `ctest` default and tsan 55/55, `check-core-deps.sh` OK, live `gomo2_*` 5/5 against real Rapfi. The owed state after a real timeout was not run against Rapfi (fake transport only).

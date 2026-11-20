---
id: DEAD-04-live-frozen-rapfi
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# Live test: a frozen Rapfi times out, then answers late and is usable again

## Scope
- One live test (`gomo2_deadline`, skipped without `PYGOMO_RAPFI`): `DeadlineEngine` over `GomocupEngine` over `ProcessTransport` with the real Rapfi. After `startSearch` the child gets `SIGSTOP`; the sink gets `ErrorEvent{"engine did not answer"}`; `startSearch` is `Busy`; after `SIGCONT` the late move is dropped, then a new search returns a legal move.
- Register it in `tests/CMakeLists.txt` with the other live tests.

## Don't
- Change production code (if the test shows a defect, stop and report it as a new todo).
- Add a pid accessor to `ProcessTransport`: the test finds its child through `/proc`.

## Traps
- Linux only (`/proc`); the live tests are already `if(UNIX)`.
- The test must `SIGCONT` the child before closing, and leave no process behind (`pgrep pbrain`).

## Done-when
Acceptance in `docs/features.json` (`"todo": "DEAD-04"`).

## Links
Source: handoff uncertain point "timeout path never ran against a real engine".

## Evidence
Real Rapfi (`pbrain-rapfi`, Rapfi_V2 build): `PYGOMO_RAPFI=... ctest -R gomo2_deadline` passes 10x in a row on default; the whole live set `gomo2_*` 6/6 x3 on tsan; the frozen child (SIGSTOP) gives the ErrorEvent, `Busy`, and after SIGCONT the late move is dropped and a new search returns a legal move. Without PYGOMO_RAPFI the test is skipped. `ctest` 62/62 on default and tsan, `check-core-deps.sh` and `check-ctest-coverage.sh` OK, no Rapfi process left. Production code unchanged.

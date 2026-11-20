---
id: PORT-02-blocking-test-sink-lifetime
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# port_blocking test: worker uses the sink after search() returned

## Scope
- Reproduce the `port_blocking` hang under `ctest --preset tsan` (seen 2026-11-20: hung over 22 min, passed alone 3/3).
- Suspected cause (from reading, not confirmed): in `tests/engine/blocking_search_test.cpp:27` the threaded `ScriptedEngine` worker calls `s.onEvent(InfoEvent{})` after `BestMoveEvent`. By then `search()` may have returned and its stack `CollectingSink` is destroyed; the worker is joined only in `~ScriptedEngine`. Use-after-free on the sink's mutex.
- Fix the test so the worker never touches a destroyed sink, keeping the "event after the terminal one is ignored" check (e.g. send it in the sync variant only, or keep the sink alive).
- Follow `bugfix.md`: regression check that stays, fix-log entry.

## Don't
- Do not change the `EventSink` / `EnginePort` contract (whether an engine may call the sink after a terminal event). That is an interface decision: record it as a planning (size L) if needed.
- Do not weaken or remove the existing assertions.

## Traps
- The hang also appeared while two ctest runs overlapped (handoff note); rule that out when reproducing.
- gdb could not attach to the hung process (ptrace), so no stack was captured.

## Done-when
Acceptance in `docs/features.json` (`"todo": "PORT-02"`), proposed and owner-approved before code.

## Evidence

## Links
`docs/todo/PORT-01-engine-port-event-sink.md`, `src/engine/blocking_search.cpp`, feature `GTP-04-deps` (its `checked_by` runs the tsan preset).

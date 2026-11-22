---
id: AUD-05-terminal-callback-order
type: audit
size: M
status: active
owner: codex
created: 2026-11-22
supersedes: []
links: [CORE-02-serialize-deadline-callbacks, AUD-03-deadline-in-decorator]
---
# DeadlineEngine claims the terminal event before delivering it

## Context

The watchdog and the wrapped engine run on different threads. An in-flight progress
callback can still be using the sink when the watchdog reaches the timeout.

## Decision

Claim the terminal event under the state mutex and wait for active progress callbacks to
return. Invoke the terminal callback with the mutex unlocked, so the sink can call
`startSearch` from it. Track both pending delivery and the owed engine answer. A late answer
can clear the debt while pending delivery still keeps the decorator `Busy`.

## Status

Active. [CORE-02 regression](../../tests/engine/deadline_engine_test.cpp).

---
id: AUD-03-deadline-in-decorator
type: audit
size: S
status: active
owner: claude
created: 2026-11-20
supersedes: [AUD-01-late-move-owed-answer]
links: [engine-search-deadline]
---
# The search deadline and the owed answer live in DeadlineEngine, not in GomocupEngine
A non-bug decision or review. Immutable; supersede it with a new one.

## Context
AUD-01 made `GomocupEngine` refuse new searches (`NotReady`) after a timeout until the late answer arrived. Planning `engine-search-deadline` (approved 2026-11-20) moved the deadline into `DeadlineEngine` so GTP and KataGomo get it too.

## Decision
`DeadlineEngine` (`include/pygomo/engine/deadline_engine.hpp`) owns the deadline, the `stop()` call and the owed-answer state. It drops the late events of a timed-out search and returns `Busy` without calling the inner engine until the late terminal event arrives. The reasoning of AUD-01 stands (no sync command works while Rapfi thinks); only the place and the error (`Busy`, owner choice) changed. `GomocupEngine` has no thread and no timer.

## Status
active

## Supersedes
AUD-01-late-move-owed-answer

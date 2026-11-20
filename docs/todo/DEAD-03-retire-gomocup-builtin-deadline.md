---
id: DEAD-03-retire-gomocup-builtin-deadline
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: engine-search-deadline
---
# GomocupEngine: retire the built-in deadline, use DeadlineEngine

## Scope
- Remove the watchdog thread, `Options`, `owed_` and deadline fields from `GomocupEngine` (GOMO-05/06 code); update its class comment.
- Move the `gomo_deadline` cases to run through `DeadlineEngine` + `GomocupEngine` with a fake transport, same cases; late move after timeout now gives `Busy`, not `NotReady`.
- Record in `docs/audit/AUD-01` follow-up note (new audit superseding it) that the decision now lives in the decorator.

## Don't
- Weaken a case to make it pass; cases whose expected error changes (`NotReady` -> `Busy`) are named in the todo evidence.

## Traps
- Live Rapfi tests (`PYGOMO_RAPFI`) still run `GomocupEngine` directly; they must pass after the removal.
- `YXSTOP` now comes from `inner.stop()`, which writes it only while a search runs.

## Done-when
Acceptance in `docs/features.json` (`"todo": "DEAD-03"`).

## Links
Planning: `features/engine-search-deadline/planning.md` (approved 2026-11-20). Audit: `docs/audit/AUD-01-late-move-owed-answer.md`.

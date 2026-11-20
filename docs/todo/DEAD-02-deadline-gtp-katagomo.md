---
id: DEAD-02-deadline-gtp-katagomo
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: engine-search-deadline
---
# DeadlineEngine over GtpEngine and KataGomoEngine (fake-transport tests)

## Scope
- Tests that wrap `GtpEngine` and `KataGomoEngine` (fake transports) in `DeadlineEngine`: silent engine -> `ErrorEvent`, a late reply (GTP with ids, KataGomo without) is dropped and the next search works, `stop()` no-op does not break the flow.
- Fix only what these tests show to be wrong in the decorator.

## Don't
- Change `GtpEngine` or `KataGomoEngine` (if a test needs it, stop and say so).

## Traps
- GTP replies to setup commands arrive before the genmove reply; a late genmove reply after a timeout must not be taken by the next search.

## Done-when
Acceptance in `docs/features.json` (`"todo": "DEAD-02"`).

## Links
Planning: `features/engine-search-deadline/planning.md` (approved 2026-11-20). Audit: `docs/audit/AUD-01-late-move-owed-answer.md`.

## Evidence
`dead2_*` (4 cases, both adapters over a fake transport) pass 20x on default and tsan; `ctest` default and tsan 61/61, `check-core-deps.sh` and `check-ctest-coverage.sh` OK. No change was needed in `GtpEngine` or `KataGomoEngine`. Not run against a real GTP or KataGomo engine.

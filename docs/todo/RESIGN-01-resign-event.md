---
id: RESIGN-01-resign-event
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: core-message-resign-events
---
# ResignEvent in core and GtpEngine

## Scope
- Add terminal `ResignEvent` to `SearchEvent`; port contract comment lists it. GtpEngine maps `resign` to it instead of `ErrorEvent{"engine resigned"}`.
- Update the existing test that expects the `ErrorEvent` (expectation change only). Do after MSG-01.

## Don't
- Nothing outside this scope; keep the build green; core stays free of I/O and protocol headers.

## Done-when
Acceptance in `docs/features.json` (`"todo": "RESIGN-01"`), proposed and owner-approved before code.

## Evidence
2026-11-20: `gtp2_result` resign case now expects `ResignEvent{}` (only change to tests besides the new alternative in the `search_event_test` visitor); default 46/46 (3 live skipped), tsan 46/46, `check-core-deps.sh` OK.

## Links
`features/core-message-resign-events/planning.md`

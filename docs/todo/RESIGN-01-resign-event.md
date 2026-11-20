---
id: RESIGN-01-resign-event
type: todo
size: M
status: backlog
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
(fill in before marking done)

## Links
`features/core-message-resign-events/planning.md`

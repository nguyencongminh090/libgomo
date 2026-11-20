---
id: MSG-01-message-event
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: core-message-resign-events
---
# MessageEvent in core and parsers

## Scope
- Add non-terminal `MessageEvent{text}` to `SearchEvent`. Gomocup parser emits it for `MESSAGE <text>` that is not an info shape; `UnparsedEvent` then means only an unknown line.
- Adapters treat it as non-terminal (GomocupEngine, KataGomoEngine). Update the tests that expected `UnparsedEvent` for MESSAGE lines (owner-approved expectation change only).

## Don't
- Nothing outside this scope; keep the build green; core stays free of I/O and protocol headers.

## Done-when
Acceptance in `docs/features.json` (`"todo": "MSG-01"`), proposed and owner-approved before code.

## Evidence
2026-11-20: `parse_*` pass (MESSAGE text -> MessageEvent, OK/ERRORS/DEBUG/empty stay Unparsed); `gomo_*`, `kata_*`, `blocking_*` pass; default 46/46 (3 live skipped), tsan 46/46, `check-core-deps.sh` OK. Tests changed: only MESSAGE expectations Unparsed -> Message (parser fixtures, `rapfi_format`, `kata_events`); `search_event_test` visitor and live `Terminal` got the new alternative.

## Links
`features/core-message-resign-events/planning.md`

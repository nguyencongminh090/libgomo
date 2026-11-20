---
id: MSG-01-message-event
type: todo
size: M
status: backlog
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
(fill in before marking done)

## Links
`features/core-message-resign-events/planning.md`

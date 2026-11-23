---
id: EXT-11-adr-guide-architecture
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# ADR, developer guide and architecture for the extension points

## Scope
Write an ADR for the changed contracts (`SearchEvent` gains CustomEvent and isTerminal, `Rules` and `Position` gain extensions, `RequestEncoder`, `CommandPort`). Add a guide section "Write your own commands, rules and board layouts" with the fake-Rapfi example. Update `docs/design/architecture.md` (and its drawio) with the new interfaces.

## Don't
No code change.

## Traps
The ADR is immutable: write it last, when the code is settled. Re-check the line links in `architecture.md` (handoff).

## Done-when
Features `EXT-11-*` in `docs/features.json` (`harness-track EXT-11`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

---
id: EXT-11-adr-guide-architecture
type: todo
size: M
status: done
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
2026-11-23, Sonnet subagent. harness-check 0 errors; full suite 118/118 on master. ADR-02-extensible-engine-commands; guide section 7 (example compiled with project flags and run over a fake transport; the ProcessTransport wiring snippet not run); PDF rebuilt (29 pages); architecture.md, drawio and PNG updated; links fixed (gomocup_engine.cpp#L136-L152, deadline_engine.cpp#L78-L107). Mermaid diagram not render-checked (no chrome-headless-shell). Guide example uses BOARD with walls as x,y,3 (a block clears INFO WALL, AUD-08).

## Links
`features/extensible-engine-commands/planning.md`

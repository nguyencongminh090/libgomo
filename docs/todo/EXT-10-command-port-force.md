---
id: EXT-10-command-port-force
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# CommandPort: typed commands, completion modes and force

## Scope
Add `CommandPort`, `Command` (name, lines, typed args, optional `block` and `terminator`), completion `None | Search | Until(predicate)`, `CommandSink` use of `EventSink`, and `CommandOptions::force`. Default: `Busy` while a search runs, nothing sent. `force`: call `stop()` (Gomocup: `YXSTOP`), wait for the old search's terminal event (it still reaches the old sink), then send the command from the reader thread; one pending command, a second gets `Busy`. `GomocupEngine` implements it; `DeadlineEngine` forwards; GTP and KataGomo implement it with `force` waiting for the natural end. Fake-Rapfi test with `INFO VCTHREAD 2` (None) and `YXANALZ` (Search, block of coordinates ending in `DONE`).

## Don't
Do not add `execute` to `EnginePort`. Do not send a command before the terminal event: Rapfi silently drops commands while thinking.

## Traps
The queued command is written from inside the listener (allowed by ProcessTransport). A terminal event delivered by DeadlineEngine's timeout must also release the pending command. Closing the engine drops it.

## Done-when
Features `EXT-10-*` in `docs/features.json` (`harness-track EXT-10`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

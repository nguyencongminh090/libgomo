---
id: EXT-10-command-port-force
type: todo
size: M
status: active
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
2026-11-23, Opus subagent; reviewed by the `reviewer` agent (5 findings: GTP sink called under writeMutex_, forced command stuck after a failed write, YXSTOP inside another thread's BOARD block, DeadlineEngine accepting unsendable commands, GTP reply doc), all fixed with regression tests that fail without the fix (c26fa30). `ctest -R '^ext10_'` 5/5 (also --repeat until-fail:10); full suite 118/118. EXT-10-tsan NOT passing: the tsan preset (-O2 -Werror, g++ 15) fails to build on master already since EXT-02 (`free-nonheap-object` in blocking_search.cpp via Value), plus `maybe-uninitialized` in deadline_engine.cpp (copy of Command with std::function). With `-DPYGOMO_WARNINGS_AS_ERRORS=OFF` (command line only): 118/118 and ext10_ x20, no ThreadSanitizer report. Decisions: wire format via commandLines(); GTP supports None only; commands get no deadline; a kept command fails after a DeadlineEngine timeout; CommandPort::canExecute added; Gomocup now serializes writes (stop/sendRaw/execute wait for a script being written).

## Links
`features/extensible-engine-commands/planning.md`

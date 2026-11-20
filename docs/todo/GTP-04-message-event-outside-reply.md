---
id: GTP-04-message-event-outside-reply
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
---
# GtpEngine: text outside a reply becomes MessageEvent

## Scope
- While a search runs, `GtpEngine::onLine` sends a line that is not part of a reply (no `=`/`?` header, not inside a reply) to the sink as a non-terminal `MessageEvent{line}`, text kept raw. The line is not parsed.
- Outside a search such lines are still dropped (banner at start-up, late output after the search ended).
- No change to `parseGtp*`, core types or the reply framing.

## Don't
- Do not touch `info string` or the analyze parser; not decided.
- Do not edit or remove an existing test. Tests at `gtp_engine_test.cpp` lines 140 and 264 are outside a search and stay as they are.
- Keep the sink call outside the lock; core stays free of I/O and protocol headers.

## Done-when
Acceptance in `docs/features.json` (`"todo": "GTP-04"`), proposed and owner-approved before code.

## Links
`docs/todo/MSG-01-message-event.md`, handoff "Uncertain" (MessageEvent never emitted by GTP).

## Evidence
2026-11-20: new test `gtp4_message` (genmove and analyze subcases) failed before the change, passes after; `-R gtp` 11/11, default 49/49 (3 live skipped), tsan 49/49 on three consecutive runs, `check-core-deps.sh` OK. No existing test changed. One earlier tsan run hung in `port_blocking` while a second ctest ran in parallel; it passes alone (6/6) and the target does not link `pygomo::gtp`, so it is not from this change.

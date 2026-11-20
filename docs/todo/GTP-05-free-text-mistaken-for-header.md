---
id: GTP-05-free-text-mistaken-for-header
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# GTP: free text starting with `=` or `?` is taken as a reply header

## Scope
- `parse::parseGtpResponseHeader` (`src/parse/gtp_parser.cpp:63`) accepts any line whose first char is `=` or `?`. Free text such as `=== Loaded ===` or `? unknown option` outside a reply opens a reply in `GtpEngine::onLine`; at the next blank line `onReply` sees no id and ends the running search with `ErrorEvent{"engine reply carries no id"}`.
- Since GTP-04 such text is meant to reach the sink as `MessageEvent`, so this now shows on a real path.
- Reproduce with a test, find the right rule (GTP: `=`/`?`, optional id digits, then space or end of line), fix, regression check, fix-log entry.

## Don't
- Do not change `info` / analyze handling or `MessageEvent` delivery (GTP-04).
- Do not edit existing tests.

## Traps
- A reply without id (`=` alone, `= text`) is valid GTP and must still parse; the GtpEngine treats it as an error only because it always sends ids.
- `?` replies end the search with an error by design; only non-header text must change.

## Done-when
Acceptance in `docs/features.json` (`"todo": "GTP-05"`), proposed and owner-approved before code.

## Evidence
Reproduced with gtp5_text and gtp_header tests (both failed), fixed in parseGtpResponseHeader; default and tsan suites pass. Fix-log GTP-05-01.

## Links
`docs/todo/GTP-04-message-event-outside-reply.md`, `src/gtp/gtp_engine.cpp` (`onLine`, `onReply`).

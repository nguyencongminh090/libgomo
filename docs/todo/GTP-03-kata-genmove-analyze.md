---
id: GTP-03-kata-genmove-analyze
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: engine-protocol-port
---
# GtpEngine: `kata-genmove_analyze` streaming (source-derived)

## Scope
- `GtpEngine::setAnalyze(interval)` (not on `EnginePort`): later searches send `kata-genmove_analyze <B|W> interval <cs>`
  instead of `genmove`. Default stays `genmove`.
- The engine prints `=<id>`, then `info ...` lines, then `play X`, then a blank line (KataGomo `gtp.cpp:2006-2035`).
  `info` lines of the running search become events at once (`parseGtpAnalyzeLine`); `play X` ends the search.
- `?` reply, a final body without `play `, or a closed connection end the search with one `ErrorEvent`.
- `stop()` stays a no-op: the command still runs inside the engine's read loop.

## Deviation from the approved feature text
Feature 1 said analyze is used "when maxVisits/turnTime is set". The existing test `gtp2_*` sets both limits and expects
`genmove`, and tests may not change, so analyze is an explicit opt-in (`setAnalyze`) instead. Told to the owner.

## Don't
- No change to core, parsers, transport, GomocupEngine, KataGomoEngine.

## Traps
- Source only; no engine run. `interval` keyword form is read from the `parseAnalyzeCommand` comment, not exercised.
- Interval is in centiseconds, rounded down, at least 1.

## Evidence
2026-11-20: `gtp3_sequence/stream/errors` pass; `gtp2_*` unchanged and passing; default 46/46 (3 live skipped), tsan 46/46,
`check-core-deps.sh` OK.

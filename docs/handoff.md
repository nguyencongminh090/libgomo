---
id: handoff
type: handoff
size: S
status: active
owner: claude
created: 2026-11-19
supersedes: []
links: []
---
# Handoff
Overwritten each session. Where the work stands between agent sessions.

## Doing now
Nothing in progress. Done and committed: CORE-01, PROTO-01, PARSE-01 (`parseGomocupLine`),
PORT-01 (`EnginePort`, `EventSink`, `pygomo_engine::search`), GOMO-01 (`GomocupEngine`, tested
on a fake transport only).

## Next step
Draft the next todo (at most M, `planning: engine-protocol-port`) and get its features approved
before writing them to `docs/features.json`. Order: PROTO `ProcessTransport` (POSIX; unblocks a
real run of `GomocupEngine`); PARSE GTP parser; PROTO GTP adapter (position diffing,
`kata-get-rules` check); fixtures from real engines. Then promote
`docs/notes/2026-11-19-harness-gate-review-findings.md`.

## Uncertain
- Owner has not answered whether Rapfi may be built outside the repo for fixtures (nothing built).
- Core has no message event: MESSAGE/DEBUG text and two-move replies are `Unparsed`. Add one?
  (owner's call; core change)
- `GomocupEngine` drops lines that arrive outside a search (engine banner); `YXSTOP` is a Yixin
  extension; any `ERROR` line ends the search. None checked against a real engine.
- Gomocup `RULE 2` vs `4` rests on Rapfi code only; `Position` cannot set arbitrary stones.
- `StreamTransport` cannot interrupt a blocked read; owner of the stream must close it.
- `.codegraph/` untracked on purpose; decide whether to gitignore.

## Blockers
None.

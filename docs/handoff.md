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
Nothing in progress. Done and committed: CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01 (fake
transport only), PROC-01 (`ProcessTransport`, POSIX, `pygomo_process`).

## Next step
Draft the next todo (at most M, `planning: engine-protocol-port`) and get its features approved
before writing them to `docs/features.json`. Options: (a) run `GomocupEngine` over
`ProcessTransport` against a real engine (needs Rapfi built: owner's answer pending); (b) PARSE
GTP parser; (c) PROTO GTP adapter (position diffing, `kata-get-rules` check). Then promote
`docs/notes/2026-11-19-harness-gate-review-findings.md`.

## Uncertain
- Owner has not answered whether Rapfi may be built outside the repo for fixtures (nothing built).
- Core has no message event: MESSAGE/DEBUG text and two-move replies are `Unparsed`. Add one?
- `GomocupEngine` drops lines outside a search; `YXSTOP` is a Yixin extension; any `ERROR` ends
  the search. None checked against a real engine.
- `ProcessTransport` kills only its direct child (no process group); sets SIGPIPE to ignored
  process-wide on first start.
- Gomocup `RULE 2` vs `4` rests on Rapfi code only; `Position` cannot set arbitrary stones.

## Blockers
None.

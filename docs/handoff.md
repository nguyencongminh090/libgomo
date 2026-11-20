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
Nothing in progress. Committed and passing: GTP-03 (setAnalyze streaming), KATA-01 (KataGomoEngine, fake transport, source-derived), GOMO-03 (lower-case INFO keys, live Rapfi verified), CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01, PROC-01, GOMO-02,
GTP-01 (GTP parsers), GTP-02 (GtpEngine, fake transport, source-derived).

## Next step
Nothing queued. New work needs a todo and owner-approved features first.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`);
re-clone into the scratchpad when needed. Rapfi source: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi`.
Separate L item: message/resign events in core (planning first). Also promote `docs/notes/2026-11-19-harness-gate-review-findings.md`.

## Uncertain
- GtpEngine::stop() is a no-op: genmove blocks KataGomo's read loop. Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes (GtpEngine, KataGomoEngine) are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- No `docs/planning` file named `engine-protocol-port` exists although todos reference it.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- `GomocupEngine` drops lines outside a search; any `ERROR` ends the search.
- `ProcessTransport` kills only its direct child; sets SIGPIPE ignored process-wide.
- Core has no message event and no resign event: owner's decision.

## Blockers
None.

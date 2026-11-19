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
Nothing in progress. Done and committed: CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01, PROC-01,
GOMO-02 (Rapfi fixtures, normal/brief message parsing, live tests). Live tests run with
`PYGOMO_RAPFI=/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi/build/pbrain-rapfi ctest --preset default -R gomo2_`.

## Next step
Draft the next todo (at most M, `planning: engine-protocol-port`), get its features approved
before writing them to `docs/features.json`. Options: GTP parser then GTP adapter (needs KataGomo
`Gom2026`, shallow re-clone outside the repo for fixtures; ask the owner for a build). Separate L
item: message/two-move events in core (planning first). Also promote
`docs/notes/2026-11-19-harness-gate-review-findings.md`.

## Uncertain
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any fixture.
- `GomocupEngine` drops lines outside a search; any `ERROR` ends the search.
- `ProcessTransport` kills only its direct child; sets SIGPIPE ignored process-wide.
- Gomocup `RULE 2` vs `4` rests on Rapfi code only; `Position` cannot set arbitrary stones.
- Core has no message event (MESSAGE/DEBUG, two-move reply stay `Unparsed`): owner's decision.

## Blockers
None.

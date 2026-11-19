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
GOMO-02, GTP-01 (GTP parsers), GTP-02 (GtpEngine over a fake transport, source-derived). Live Rapfi tests:
`PYGOMO_RAPFI=/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi/build/pbrain-rapfi ctest --preset default -R gomo2_`.

## Next step
Draft the next todo (at most M, `planning: engine-protocol-port`) and get its features approved before writing
them to `docs/features.json`. Options: a GTP analyze stream (`kata-genmove_analyze` gives InfoEvents but is also synchronous, so still
not interruptible; only `kata-analyze` runs in the background and stops on the next input line); read KataGomo `cpp/command/gomprotocol.cpp` (a Gomocup-style protocol, not
read yet); promote `docs/notes/2026-11-19-harness-gate-review-findings.md`. Separate L item: message/resign
events in core (planning first). Reference source (outside the repo, may be gone): KataGomo `Gom2026` @ `9a0e65c`.

## Uncertain
- GtpEngine::stop() is a no-op: genmove blocks KataGomo's read loop. Limits persist in the engine between searches.
- GTP shapes are read from source only; no real GTP engine was ever run. VCN rules and `kata-set-rule` not mapped.
- No `docs/planning` file named `engine-protocol-port` exists although todos reference it.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- `GomocupEngine` drops lines outside a search; any `ERROR` ends the search.
- `ProcessTransport` kills only its direct child; sets SIGPIPE ignored process-wide.
- Core has no message event and no resign event: owner's decision.

## Blockers
None.

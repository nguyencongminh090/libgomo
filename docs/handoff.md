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
GOMO-02, GTP-01 (GTP header/vertex/analyze parsers, source-derived fixtures). Live Rapfi tests:
`PYGOMO_RAPFI=/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi/build/pbrain-rapfi ctest --preset default -R gomo2_`.

## Next step
GTP-02: `GtpEngine` adapter (EnginePort over Transport, framing by blank line, `play`/`genmove`/`boardsize`/
`clear_board`, stop by newline during analyze). No GTP engine exists, so test it with a fake transport that
replays the source-derived shapes. Draft the todo (at most M, `planning: engine-protocol-port`) and get its
features approved before writing them to `docs/features.json`.
Reference source (outside the repo, may be gone): KataGomo `Gom2026` @ `9a0e65c`, `cpp/command/gtp.cpp`.
It also has `cpp/command/gomprotocol.cpp` (a Gomocup-style protocol), not read yet.

## Uncertain
- GTP shapes are read from source only; no real GTP engine was ever run. Rules mapping (`kata-set-rule`) not read.
- No `docs/planning` file named `engine-protocol-port` exists although todos reference it.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- `GomocupEngine` drops lines outside a search; any `ERROR` ends the search.
- `ProcessTransport` kills only its direct child; sets SIGPIPE ignored process-wide.
- Core has no message event and no resign event: owner's decision.

## Blockers
None.

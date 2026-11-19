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
Nothing in progress. CORE-01 (CMake skeleton + core types) is done and committed: `Coord`, `Move`,
`Side`, `Position`, `Rules`, `Limits`, `SearchEvent` in `include/pygomo/core/`, doctest tests,
smoke test, `scripts/check-core-deps.sh`. Planning `engine-protocol-port` is approved.
Reference engines: Rapfi (local, read-only,
`/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi/`) and
KataGomo `Gom2026` (clone was in the old session scratchpad; re-clone shallow if needed).

## Next step
Create the next todos from the planning (each at most M, `planning: engine-protocol-port`) and get
their features approved by the owner. Proposed order: PROTO Transport port + `StreamTransport`;
PARSE Gomocup line parser; PROTO Gomocup adapter; PARSE GTP parser; PROTO GTP adapter (position
diffing, `kata-get-rules` check); PROTO `ProcessTransport` (POSIX); fixtures captured from real
engines (needs building Rapfi and KataGomo).

## Uncertain
- Gomocup spec meaning of `RULE 2` vs `4` rests on Rapfi code only.
- `Position` has no way to set up arbitrary stones (GTP `set_position`); add when the GTP adapter needs it.
- `.codegraph/` is left untracked on purpose (index, not source); decide whether to gitignore it.

## Blockers
None.

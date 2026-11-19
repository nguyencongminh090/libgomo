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
Nothing in progress. Done and committed: CORE-01 (core value types) and PROTO-01 (`Transport` port
+ `StreamTransport` in `pygomo_transport`, ThreadSanitizer preset `tsan`). Planning
`engine-protocol-port` is approved. Reference engines: Rapfi (local, read-only,
`/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi/`) and
KataGomo `Gom2026` (re-clone shallow when fixtures are needed).

## Next step
Draft the next todo (at most M, `planning: engine-protocol-port`) and get its features approved
by the owner before writing them to `docs/features.json`. Proposed order: PARSE Gomocup line
parser (text line to `SearchEvent`, pure, uses `Unparsed` for unknown lines); PROTO Gomocup
adapter; PARSE GTP parser; PROTO GTP adapter (position diffing, `kata-get-rules` check); PROTO
`ProcessTransport` (POSIX); fixtures captured from real engines (needs building both).
Also pending: promote `docs/notes/2026-11-19-harness-gate-review-findings.md` to a todo once the
owner picks a domain (none of CORE/PROTO/PARSE fits harness tooling); agreed to do it after the
parser work starts, not before.

Waiting on the owner: may Rapfi be built in a separate directory outside the repo to capture
real transcripts as fixtures for PARSE-01, or should the parser rely on reading `gomocup.cpp`
only? (Default if unanswered: read the code and draft the todo; do not build anything.)

## Uncertain
- Gomocup spec meaning of `RULE 2` vs `4` rests on Rapfi code only.
- `Position` cannot set arbitrary stones (GTP `set_position`); add when the GTP adapter needs it.
- `StreamTransport` cannot interrupt a blocked read; the owner of the stream must close it
  (documented in the header). `ProcessTransport` must end the process to unblock the reader.
- `.codegraph/` is left untracked on purpose; decide whether to gitignore it.

## Blockers
None.

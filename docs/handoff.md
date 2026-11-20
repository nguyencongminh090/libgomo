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
Nothing in progress. Committed and passing: CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01, PROC-01, GOMO-02,
GTP-01 (GTP parsers), GTP-02 (GtpEngine, fake transport, source-derived).
Owner approved GOMO-03 features this session; they are in `docs/features.json` with `passes: false`. No code written yet.

## Next step
Implement `GOMO-03` (todo `docs/todo/GOMO-03-lowercase-info-keys.md`, status backlog -> active -> done):
1. Read the todo, run the Smoke command, read `src/gomocup/gomocup_engine.cpp:54-59`.
2. Change the six `INFO` lines there to lower-case keys (`INFO rule`, `max_moves`, `timeout_turn`, `timeout_match`, `max_depth`, `max_node`).
3. Apply exactly the test and feature-description edits listed in the todo ("Approved ... changes"); nothing else.
4. Check: `ctest --preset default`, live Rapfi
   `PYGOMO_RAPFI=/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi/build/pbrain-rapfi ctest --preset default -R gomo2_`,
   `ctest --preset tsan`, `scripts/check-core-deps.sh`, `node scripts/harness/harness-check.mjs`. Flip `passes`, fill Evidence, update TODO.md, commit.
Then, one per session, each needing its features approved by the owner before they go to `docs/features.json`:
- `KATA-01`: separate `KataGomoEngine` for KataGomo `gom` mode (owner chose a separate adapter). Facts in
  `docs/notes/2026-11-20-kata-gomprotocol-findings.md`: `START`/`BOARD`..`DONE` -> `x,y`; rules by `FREESTYLE`/`STANDARD`/`RENJU`
  and `maxmoves N`; `INFO timeout_turn MS`; `maxNodes` via `kata-set-param maxVisits`; success prints nothing, errors print
  `ERROR ...`. Ask the owner: map `matchTime` to `INFO time_left`? (default proposal: Unsupported).
- `GTP-03`: `kata-genmove_analyze` in `GtpEngine` (owner: it is a GTP extension, support it). Streams `info` lines as `InfoEvent`s,
  final `play X`. Needs streaming framing (today a reply is handled only at the blank line). Still synchronous, so `stop()` stays a no-op.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`);
re-clone into the scratchpad when needed. Rapfi source: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi`.
Separate L item: message/resign events in core (planning first). Also promote `docs/notes/2026-11-19-harness-gate-review-findings.md`.

## Uncertain
- Rapfi accepting lower-case `INFO` keys is read from source (`command/gomocup.cpp:191`), not yet run: GOMO-03-live proves it.
- GtpEngine::stop() is a no-op: genmove blocks KataGomo's read loop. Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- No `docs/planning` file named `engine-protocol-port` exists although todos reference it.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- `GomocupEngine` drops lines outside a search; any `ERROR` ends the search.
- `ProcessTransport` kills only its direct child; sets SIGPIPE ignored process-wide.
- Core has no message event and no resign event: owner's decision.

## Blockers
None.

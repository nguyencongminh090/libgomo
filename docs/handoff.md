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
Nothing in progress. Committed and passing: GTP-05 (header needs a space or end of line after the id), PORT-02 (test worker no longer uses a destroyed sink; tsan x50 passes), GTP-04 (text outside a reply during a search -> MessageEvent; follow-up 0136bdc tests text between replies; d1c2377 registers gtp4_message with ctest, it was never run before), PROC-02 (process group kill, thread-local SIGPIPE), HARNESS-01..04 (gate hardening), RESIGN-01 (ResignEvent), MSG-01 (MessageEvent), GTP-03 (setAnalyze streaming), KATA-01 (KataGomoEngine, fake transport, source-derived), GOMO-03 (lower-case INFO keys, live Rapfi verified), CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01, PROC-01, GOMO-02,
GTP-01 (GTP parsers), GTP-02 (GtpEngine, fake transport, source-derived).

## Next step

TEST-01 in backlog: script that fails when ctest runs no entry for a doctest case (fix-log GTP-04-01); needs owner-approved features.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`);
re-clone into the scratchpad when needed. Rapfi source: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi`.


## Uncertain
- GtpEngine::stop() is a no-op: genmove blocks KataGomo's read loop. Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes (GtpEngine, KataGomoEngine) are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- No `docs/planning` file named `engine-protocol-port` exists although todos reference it.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- `GomocupEngine` drops lines outside a search; any `ERROR` ends the search.
- `ProcessTransport` signals its process group only when it escalates (SIGTERM/SIGKILL); grandchildren of a child that exits on its own are left running. The child no longer gets terminal Ctrl-C.
- GtpEngine emits `MessageEvent` only for text outside a reply during a search; `info string` lines and unparsable analyze lines still give no MessageEvent (not decided, KataGomo output not checked).
- `GTP-04-deps` tsan check no longer hangs after PORT-02 (owner may confirm it can stay passing).

- Harness gate: `canonicalRel` always lower-cases (fail closed), so case variants ask even on a case-sensitive FS. `features/*/planning.md` now asks the owner (added beyond the todo text). `scripts/harness` is a manual copy of the skill scripts; re-sync after skill changes.

## Blockers
None.

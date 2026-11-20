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
Nothing in progress. Committed and passing: GOMO-04 (ERROR line during a search -> MessageEvent, search stays open), PROC-03 (SIGKILL to the process group also when the child exited by itself, leader still a zombie), TEST-01 (scripts/check-ctest-coverage.sh, ctest entry ctest_coverage), GTP-05 (header needs a space or end of line after the id), PORT-02 (test worker no longer uses a destroyed sink; tsan x50 passes), GTP-04 (text outside a reply during a search -> MessageEvent; follow-up 0136bdc tests text between replies; d1c2377 registers gtp4_message with ctest, it was never run before), PROC-02 (process group kill, thread-local SIGPIPE), HARNESS-01..04 (gate hardening), RESIGN-01 (ResignEvent), MSG-01 (MessageEvent), GTP-03 (setAnalyze streaming), KATA-01 (KataGomoEngine, fake transport, source-derived), GOMO-03 (lower-case INFO keys, live Rapfi verified), CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01, PROC-01, GOMO-02,
GTP-01 (GTP parsers), GTP-02 (GtpEngine, fake transport, source-derived).

## Next step

Backlog empty; ask the owner.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`);
re-clone into the scratchpad when needed. Rapfi source: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi`.


## Uncertain
- GtpEngine::stop() is a no-op: genmove blocks KataGomo's read loop. Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes (GtpEngine, KataGomoEngine) are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- GomocupEngine drops lines outside a search (by design, no sink). An `ERROR` during a search is now a MessageEvent (GOMO-04); a real failure with no move leaves the search open until close(), no timeout. `KataGomoEngine` still ends the search on `ERROR` (KATA-01 feature).
- GtpEngine: text outside a reply during a search -> `MessageEvent`; unparsable analyze lines -> `UnparsedEvent` (kept, not lost). KataGomo (Gom2026 @ checked 2026-11-20) never prints `info string`, so nothing more is planned.
- `GTP-04-deps` tsan check no longer hangs after PORT-02 (owner may confirm it can stay passing).

- ProcessTransport: the child no longer gets terminal Ctrl-C (own process group). Processes that call setsid in the group escape the kill.
- Harness gate: `canonicalRel` always lower-cases (fail closed), so case variants ask even on a case-sensitive FS. `features/*/planning.md` now asks the owner (added beyond the todo text). `scripts/harness` is a manual copy of the skill scripts; re-sync after skill changes.

## Blockers
None.

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
Nothing in progress. Last session (2026-11-22) closed two todos, both reviewed by Claude and
implemented by Codex:
- CORE-02 done (`e0f30ae`, `481a1d8`): `DeadlineEngine` delivers a terminal event only after
  in-flight progress callbacks return (AUD-05, fix-log CORE-02-01). Features `CORE-02-order`
  and `CORE-02-deps` pass; 66/66 on default and tsan.
- CORE-03 done (`84ba5e9`..`2bcf276`, `102c955`): `docs/design/architecture.md` (design id
  `CORE-05-overall-architecture`, so the next CORE todo is CORE-06) with Draw.io/PNG and
  Mermaid: CMake dependency graph incl. `parse -> core`, and the `ProcessTransport` session
  lifecycle incl. the close-time `ErrorEvent`. Harness: 0 findings.
- Align pass: `f0cbbde`, fixed up in `aa926a3` (member comments moved to their own lines).

## Next step
1. CORE-04 (backlog): reproduce first. A failed inner `startSearch` clears `sink_`/`deadline_`
   without checking `searchId_` (`deadline_engine.cpp:55`). Under the `EnginePort` contract it
   may only happen with a contract-breaking engine; the owner decides after the repro.
2. Planning `detailed-search-analysis` (draft, size L): GTP drops later candidates and Gomocup
   drops MultiPV ranks when building `InfoEvent`. The owner must choose the public
   representation before todos are split. Its `created: 2026-11-25` is a future date; fix it.

Doc skills in `.claude/skills/`: README files of `documentation` and `writing-clearly-and-concisely` were read in DOC-02 (nothing to adapt); the other copied skills still carry unread README.WARNING.md/README.md.
Style normalization (planning code-style-normalization) is finished; the owner may close the planning. A `.clang-format` stays a separate planning.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`); re-clone into the scratchpad when needed. Rapfi: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi` (binary `build/pbrain-rapfi`).

## Uncertain
- DeadlineEngine (CORE-02): a progress callback that never returns also holds back the timeout
  error and the destructor; one that makes the wrapped engine deliver its terminal event on the
  same thread deadlocks (documented in the header, not guarded).
- `dead1_serialization` assumes the watchdog reaches the timeout within a 250 ms window; 72
  parallel tsan runs passed, but a very loaded machine could still flake it.
- `docs/design/architecture.md` links `deadline_engine.cpp#L62-L76` and
  `gomocup_engine.cpp#L133`; re-check them when those files change.
- Guide pre-existing source lines over 100 columns (about 18) were left alone (no restyle outside changed text).
- check-doc-comments.sh is a heuristic: it checks types, aliases, free functions and public methods, not data members or enumerators (those are reviewed by hand). Overrides need their own `///` line each; accepting one group comment for them in the script would loosen the rule, so it needs an owner decision.
- GtpEngine::stop() is a no-op by protocol: docs/audit/AUD-02 (KataGomo reads stdin on one thread; genmove cannot be interrupted). Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes (GtpEngine, KataGomoEngine) are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- GomocupEngine drops lines outside a search (by design, no sink). An `ERROR` during a search is a MessageEvent (GOMO-04); a search with turnTime ends with ErrorEvent "engine did not answer" only when wrapped in DeadlineEngine (DEAD-01..03); unwrapped there is no timeout. `KataGomoEngine` still ends the search on `ERROR` (KATA-01 feature).
- GtpEngine reply attribution assumes one reply per non-empty, non-comment line (AUD-04 Cost). Untested: a partial write failure in startSearch (i > 0), and stale replies of a failed search with a raw command between them (traced by the reviewer, correct).
- GtpEngine: text outside a reply during a search -> `MessageEvent`; unparsable analyze lines -> `UnparsedEvent` (kept, not lost). KataGomo (Gom2026 @ checked 2026-11-20) never prints `info string`, so nothing more is planned.
- `GTP-04-deps` tsan check no longer hangs after PORT-02 (owner may confirm it can stay passing).
- ProcessTransport: the child no longer gets terminal Ctrl-C (own process group). Processes that call setsid in the group escape the kill.
- Harness gate: `canonicalRel` always lower-cases (fail closed), so case variants ask even on a case-sensitive FS. `features/*/planning.md` asks the owner. `scripts/harness` is a manual copy of the skill scripts; re-sync after skill changes.
- DeadlineEngine: an engine that never answers after a timeout stays Busy; the caller must close it (docs/audit/AUD-03). The Gomocup timeout path ran against a real Rapfi (frozen with SIGSTOP, DEAD-04); GTP and KataGomo only against fake transports.

## Blockers
None.

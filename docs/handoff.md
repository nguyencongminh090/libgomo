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
Nothing in progress. Planning `clang-format` is done (2026-11-23): FMT-01..04 all done. `.clang-format` is full style
(sorted includes), `scripts/check-format.sh` runs as ctest `style_format`, `coding-style.md` §1 says clang-format owns layout
and `/align` is retired. Measurements: AUD-06 (alignment-only), AUD-07 (full style). ctest 83/83 pass (live tests skipped).

## Next step
1. Plannings `detailed-search-analysis` and `code-style-normalization` are closed (2026-11-23,
   all their todos done; frontmatter stays `approved`, the harness has no closed status).
   No other todo is queued in this handoff; pick from `TODO.md`.
   `agy -p` in `--mode accept-edits` edits files but cannot run shell commands headless;
   `--dangerously-skip-permissions` is blocked here, so the orchestrator builds and tests.

Doc skills in `.claude/skills/`: README files of `documentation` and `writing-clearly-and-concisely` were read in DOC-02 (nothing to adapt); the other copied skills still carry unread README.WARNING.md/README.md.
Style normalization (planning code-style-normalization) is finished and closed (2026-11-23); formatting is now owned by clang-format (planning clang-format, closed).
FMT-04 done (2026-11-23): ctest `style_format`, coding-style rule rewritten, `/align` retired. Planning `clang-format` has all its todos done.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`); re-clone into the scratchpad when needed. Rapfi: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi` (binary `build/pbrain-rapfi`).

## Uncertain
- DeadlineEngine (CORE-02): a progress callback that never returns also holds back the timeout
  error and the destructor; one that makes the wrapped engine deliver its terminal event on the
  same thread deadlocks (documented in the header, not guarded).
- `dead1_serialization` assumes the watchdog reaches the timeout within a 250 ms window; 72
  parallel tsan runs passed, but a very loaded machine could still flake it.
- `docs/design/architecture.md` links `deadline_engine.cpp#L67-L81` and
  `gomocup_engine.cpp#L138`; re-check them when those files change.
- Guide pre-existing source lines over 100 columns (about 18) were left alone (no restyle outside changed text).
- check-doc-comments.sh is a heuristic: it checks types, aliases, free functions and public methods, not data members or enumerators (those are reviewed by hand). Overrides need their own `///` line each; accepting one group comment for them in the script would loosen the rule, so it needs an owner decision.
- GtpEngine::stop() is a no-op by protocol: docs/audit/AUD-02 (KataGomo reads stdin on one thread; genmove cannot be interrupted). Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes (GtpEngine, KataGomoEngine) are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- GomocupEngine drops lines outside a search (by design, no sink). An `ERROR` during a search is a MessageEvent (GOMO-04); a search with turnTime ends with ErrorEvent "engine did not answer" only when wrapped in DeadlineEngine (DEAD-01..03); unwrapped there is no timeout. `KataGomoEngine` still ends the search on `ERROR` (KATA-01 feature).
- GtpEngine reply attribution assumes one reply per non-empty, non-comment line (AUD-04 Cost). Untested: stale replies of a failed search with a raw command between them (traced by the reviewer, correct).
- GtpEngine: text outside a reply during a search -> `MessageEvent`; unparsable analyze lines -> `UnparsedEvent` (kept, not lost). KataGomo (Gom2026 @ checked 2026-11-20) never prints `info string`, so nothing more is planned.
- `GTP-04-deps` tsan check no longer hangs after PORT-02 (owner may confirm it can stay passing).
- ProcessTransport: the child no longer gets terminal Ctrl-C (own process group). Processes that call setsid in the group escape the kill.
- Harness gate: `canonicalRel` always lower-cases (fail closed), so case variants ask even on a case-sensitive FS. `features/*/planning.md` asks the owner. `scripts/harness` is a manual copy of the skill scripts; re-sync after skill changes.
- DeadlineEngine: an engine that never answers after a timeout stays Busy; the caller must close it (docs/audit/AUD-03). The Gomocup timeout path ran against a real Rapfi (frozen with SIGSTOP, DEAD-04); GTP and KataGomo only against fake transports.

## Blockers
None.

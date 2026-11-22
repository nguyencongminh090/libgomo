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
Nothing in progress. Committed and passing: DOC-02 (guide pass 2: 33df804 stale facts, then the prose pass; PDF rebuilt), DOC-01 (pass 1, 8890484). Before them: STYLE-01..04, DEAD-01..04, GOMO-01..06, GTP-01..05, PROC-01..03 and the rest (see TODO.md).

## Next step

1. DOC-03 (not yet a todo; owner creates it): redraw the three TikZ figures in `docs/guide/developer-guide.tex` (module dependencies, search data flow, DeadlineEngine timeline). Note `tex-tables-and-figures` in SKILLS_TREE.
2. GTP-06 (backlog, found in DOC-02): an id-less reply to `GtpEngine::sendRaw` ends a running or the next search. Needs features from the owner, then a fake-transport reproduction first. The guide states it in Known limitations; update that bullet when fixed.
Doc skills in `.claude/skills/` still carry README.WARNING.md/README.md (read, not adapted; no project paths needed changing).
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`); re-clone into the scratchpad when needed. Rapfi: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi` (binary `build/pbrain-rapfi`).

## Uncertain
- Guide pre-existing source lines over 100 columns (about 18) were left alone (no restyle outside changed text).
- check-doc-comments.sh is a heuristic: it checks types, aliases, free functions and public methods, not data members or enumerators (those are reviewed by hand). Overrides need their own `///` line each; accepting one group comment for them in the script would loosen the rule, so it needs an owner decision.
- GtpEngine::stop() is a no-op by protocol: docs/audit/AUD-02 (KataGomo reads stdin on one thread; genmove cannot be interrupted). Limits persist in the engine between searches.
- GTP and KataGomo `gom` shapes (GtpEngine, KataGomoEngine) are read from source only; no such engine was ever run. VCN rules and `kata-set-rule` not mapped.
- Late `[Pondering]` lines and `coord_conversion_mode` != none are not covered by any Gomocup fixture.
- GomocupEngine drops lines outside a search (by design, no sink). An `ERROR` during a search is now a MessageEvent (GOMO-04); a search with turnTime ends with ErrorEvent "engine did not answer" only when wrapped in DeadlineEngine (turnTime+2s grace+2s, DEAD-01..03); unwrapped there is no timeout. `KataGomoEngine` still ends the search on `ERROR` (KATA-01 feature).
- GtpEngine: text outside a reply during a search -> `MessageEvent`; unparsable analyze lines -> `UnparsedEvent` (kept, not lost). KataGomo (Gom2026 @ checked 2026-11-20) never prints `info string`, so nothing more is planned.
- `GTP-04-deps` tsan check no longer hangs after PORT-02 (owner may confirm it can stay passing).

- ProcessTransport: the child no longer gets terminal Ctrl-C (own process group). Processes that call setsid in the group escape the kill.
- Harness gate: `canonicalRel` always lower-cases (fail closed), so case variants ask even on a case-sensitive FS. `features/*/planning.md` now asks the owner (added beyond the todo text). `scripts/harness` is a manual copy of the skill scripts; re-sync after skill changes.

- DeadlineEngine: an engine that never answers after a timeout stays Busy; the caller must close it (docs/audit/AUD-03). The Gomocup timeout path ran against a real Rapfi (frozen with SIGSTOP, DEAD-04); GTP and KataGomo only against fake transports; `DeadlineEngine` must be destroyed after the inner engine stopped delivering events.

## Blockers
None.

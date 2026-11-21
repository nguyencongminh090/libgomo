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
Nothing in progress. Committed and passing: STYLE-04 (live, smoke, main; style checks are ctest entries style_line_length and style_doc_comments), STYLE-03 (8009ad6 + align commit: engine, gomocup, gtp, katagomo), STYLE-02 (08cf296) and STYLE-01 (planning code-style-normalization): scripts/check-line-length.sh and scripts/check-doc-comments.sh; core headers documented and aligned (86d6fd9 comments + wrap, 6fa582c `style: align columns`). Before it: DEAD-04, DEAD-01..03, GOMO-06, GOMO-05, GOMO-04, PROC-03, TEST-01, GTP-05, PORT-02, GTP-04, PROC-02, HARNESS-01..04, RESIGN-01, MSG-01, GTP-03, KATA-01, GOMO-03, CORE-01, PROTO-01, PARSE-01, PORT-01, GOMO-01, PROC-01, GOMO-02,
GTP-01, GTP-02.

## Next step

Revising `docs/guide/developer-guide.tex` in three passes (owner's plan):
1. Content: DONE as DOC-01 (five parts by Diataxis type, Getting started, limitations once, PDF rebuilt).
2. Prose and structure: next. Needs its own todo (DOC-02, size M). Use skill `writing-clearly-and-concisely` and note `craft-editing-and-revision`. Also re-check the capability matrix and parser shapes against the code (not checked in pass 1).
3. Diagrams: the three TikZ figures (module dependencies, search data flow, DeadlineEngine timeline). Own todo (DOC-03).
Doc-writing skills are in `.claude/skills/` (documentation, writing-clearly-and-concisely, documentation-and-adrs, adr-author, latex-document-skill); each still has a README.WARNING.md/README.md not yet read or adapted. Notes to read in place: `SKILLS_TREE/knowledge/documents/items/` (craft-editing-and-revision, tex-tables-and-figures, tex-build-latexmk-engines).

Otherwise no todo is open. Style normalization (planning code-style-normalization) is finished; the owner may close the planning. A `.clang-format` stays a separate planning.
KataGomo source is outside the repo (shallow clone of `https://github.com/hzyhhzy/KataGomo`, branch `Gom2026`, commit `9a0e65c`);
re-clone into the scratchpad when needed. Rapfi source: `/run/media/ngmint/Data/Programming/Programming/C++/Project/Lab/Rapfi_V2/rapfi/Rapfi`.


## Uncertain
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

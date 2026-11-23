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
CORE-10 updated the README migration instructions, contributor commands, guide and architecture.
The README and guide examples built in a standalone CMake project and returned moves from Rapfi.
The guide PDF rebuilt, and the Draw.io source and PNG agree on `links gomo::core`. A clean
`default` build passed all 119 CTest entries, including the Rapfi V2 and V3 live tests.

## Next step
Close CORE-10 once its acceptance-file edits are approved and checked. CORE-08 and CORE-09
also have verified implementation and pending acceptance-file edits.

## Uncertain
- LIVE-01: YXANALZ asserts one of the two listed moves (spike saw only 9,9; 6,6 also appears).
  V3 stable and the AUD-08 drop race are not covered.
- TSAN-01: the same g++ 15 warnings also break any other -O2 build with
  GOMO_WARNINGS_AS_ERRORS=ON (e.g. a Release preset); only the tsan preset was changed.
  No GCC bug id was verified; recheck the two -Wno-error flags after a GCC upgrade.
- EXT-10: Rapfi V3 may drop a command written right after its move (AUD-08); built as planned,
  documented only (owner decision 2026-11-23). V2 YXANALZ with no valid move answers only ERROR,
  which is not terminal: Busy until close (use Until). GTP CommandPort supports None only.
- GomocupEngine now serializes writes (writeMutex_ before mutex_): stop(), sendRaw and execute
  wait while another thread writes a script.
- The illegal-move check does not reject a move on a blocked (WALL) cell; BoardLayout == is
  order-sensitive.
- The Mermaid class diagram in `docs/design/architecture.md` has not been render-checked.
- DeadlineEngine (CORE-02): a progress callback that never returns also holds back the timeout
  error and the destructor; one that makes the wrapped engine deliver its terminal event on the
  same thread deadlocks (documented in the header, not guarded).
- `dead1_serialization` assumes the watchdog reaches the timeout within a 250 ms window; 72
  parallel tsan runs passed, but a very loaded machine could still flake it.
- `docs/design/architecture.md` links `deadline_engine.cpp#L78-L107` and
  `gomocup_engine.cpp#L136-L152`; re-check them when those files change.
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
CORE-08 needs owner approval for its acceptance entries and the seven style-check header path
updates in `docs/features.json`. CORE-09 needs owner approval for its acceptance entries and
the warning and Rapfi name updates in that file. CORE-10 needs approval for its acceptance
entries and the `CORE-03-edges` label update. The diagram check passed with the new label.

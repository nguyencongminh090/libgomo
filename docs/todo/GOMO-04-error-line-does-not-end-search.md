---
id: GOMO-04-error-line-does-not-end-search
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# GomocupEngine: an ERROR line during a search is a warning, not the end

## Scope
- In `GomocupEngine::onLine`, a parsed `ErrorEvent` (line `ERROR <text>`) no longer ends the search: it reaches the sink as `MessageEvent{"ERROR <text>"}` (the line raw) and the search stays open until a move, swap or connection close.
- Real `ErrorEvent`s stay terminal: illegal move and connection closed.
- Update the class comment in `gomocup_engine.hpp`.

## Don't
- Change `parseGomocupLine` (its `ErrorEvent` for `ERROR` lines stays, PARSE-01 feature).
- Change `KataGomoEngine` (own adapter, KATA-01 feature says ERROR ends the search there); note it as a follow-up.
- Add a timeout: the owner chose "no end, report as warning". An engine that fails for real and sends no move leaves the search open until stop()/close() or connection close.

## Traps
- Rapfi prints non-fatal `ERROR` lines (`Unknown rule id ... reset`, `Max memory too small`, `Analyze move ... ignored`, `command/gomocup.cpp` 226/244/289/736/744) and then still answers with a move; the move used to be dropped.
- `blocking_search` treats `MessageEvent` as non-terminal, so it waits for the move.

## Done-when
Acceptance in `docs/features.json` (`"todo": "GOMO-04"`).

## Links
Source: handoff "Uncertain" point on `GomocupEngine`; owner decision 2026-11-20 (warning, not terminal).

## Evidence
`gomo_warning` failed first (ERROR ended the search, the move was dropped), then passes. `ctest --preset default` and `--preset tsan` 54/54 pass (live skipped, no real Rapfi run), `scripts/check-core-deps.sh` OK.

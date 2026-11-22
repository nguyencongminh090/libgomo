---
id: ADR-01-info-event-candidates
type: adr
size: L
status: active
owner: claude
created: 2026-11-22
supersedes: []
links: [detailed-search-analysis, CORE-06-info-event-candidates, GOMO-07-multipv-candidates, GTP-07-analyze-all-candidates]
---
# Per-move analysis lives in `InfoEvent::candidates`
A hard-to-reverse architecture decision. Immutable; supersede it with a new one.

## Context
`InfoEvent` (core, `include/pygomo/core/search_event.hpp`) had one summary: `depth`, `nodes`,
`score`, `elapsed`, `principalVariation`. Two engine outputs did not fit and lost data:

- Gomocup MultiPV (Rapfi): one candidate per line. `MESSAGE multipv 1 ev 7 pv 1,1` and
  `MESSAGE multipv 2 ev 7 pv 1,1` produced equal `InfoEvent`s; the rank was dropped.
- GTP analysis (KataGomo `kata-analyze` / `lz-analyze`): all candidates on one line. Only the
  first `info` entry was read, so `info move H8 visits 120 pv H8` with or without
  `info move G8 visits 50 pv G8` appended produced the same event.

Once an `InfoEvent` is built the application cannot recover what was dropped. Constraints:

- The core stays protocol-neutral and free of I/O, thread and adapter headers.
- `SearchEvent` must not gain a type. Three places treat every type other than Info, Message and
  Unparsed as the end of a search (`src/engine/blocking_search.cpp`,
  `src/engine/deadline_engine.cpp`, `src/katagomo/katagomo_engine.cpp`): a new type compiles
  and ends the search at the first candidate line.
- `parseGomocupLine` and `parseGtpAnalyzeLine` keep their signatures (one line, one event).
- A member appended at the end of `InfoEvent` keeps the defaulted `operator==` and existing
  aggregate initialisation working.

The analysis is in planning `features/detailed-search-analysis/planning.md`, approved by the
owner on 2026-11-22.

## Decision
`InfoEvent` gets, as its last member, `std::vector<Candidate> candidates` (empty if the report
has none). `Candidate` is a new core type: optional `rank` (1 = best), `move`, `score`,
`visits`, `winrate` and `drawrate` (both 0..1), and a `principalVariation`. Only typed data is
kept; the raw report line is not attached.

Mapping:

- `rank` comes only from the engine, never guessed: Gomocup `(N)` and `multipv N`; GTP
  `order k` gives `k + 1`. A GTP entry without `order` has no rank.
- Gomocup builds a `Candidate` only for a ranked line (one element). Unranked lines (`depth …
  ev … pv`, `Bestline`, `Speed …`) keep `candidates` empty.
- GTP: every `info` entry of the line becomes a `Candidate`, in printed order, so one line stays
  one snapshot. A broken first entry makes the line `UnparsedEvent`; a broken later entry keeps
  the candidates before it and stops there.
- `move`: GTP `move`; Gomocup the first move of the PV.
- `winrate`/`drawrate`: Rapfi `W`/`D` and `w`/`d` percentages divided by 100. GTP: an entry with
  `utility` is on the 0..1 scale (kata), otherwise divided by 10000 (lz). Values outside 0..1
  are left empty. GTP has no standard draw rate: `scoreLead`/`scoreMean` are not read (KataGomo
  stores its draw rate there, unlike KataGo).

Meaning of the summary fields of `InfoEvent`:

- `depth`, `elapsed`: of the whole search, as before.
- `nodes`: only the search total (Rapfi `n`, `Node`, the `Visit` of a `Speed` line). Visits spent
  on one move (`V`, `v`, GTP `visits`) live only in `Candidate::visits`.
- `score`, `principalVariation`: only from an unranked line or the rank-1 candidate; a rank-2+
  line leaves both empty. A GTP line where no entry has `order` takes them from the first entry
  (convention, keeps the earlier behaviour).

Why: with `candidates` available, a summary holding rank-2 data or one move's visits would mislead
an application that reads only the summary (it would take it for the best line or the total node
count). Rapfi always prints the node total on its own line, so the summary loses nothing.

Rejected options (numbered as in the planning; option 1 is the decision above):

- Option 2: `std::optional<int> multipv` on `InfoEvent`, one event per candidate. Smaller, but
  `parseGtpAnalyzeLine` would return several events (public API change) and the application
  would lose the GTP snapshot boundary.
- Option 3: attach the raw report line. The application would still have to know each engine's
  syntax to read a candidate.
- Option 4: a new `Candidate` type in `SearchEvent`. It would end searches at the first
  candidate line (see the three terminal checks above).

## Consequences
- `InfoEvent` and `Candidate` are public core interface; changing them again is size L.
- Behaviour change for summary readers: Gomocup rank-2+ lines no longer fill `score` and
  `principalVariation`, and per-move visits no longer appear as `nodes` (GTP analysis `nodes` is
  now always empty). The existing test assertions the planning lists moved to `candidates`.
- Gomocup lines are not grouped into snapshots: the framework does not guess iteration
  boundaries; the application groups ranked lines itself.
- Winrate perspective is whatever the engine reports: KataGomo follows its
  `reportAnalysisWinratesAs` setting (default: side to move), which the framework cannot see.
- Not covered: `prior`, `lcb`, `utility`, `seldepth`, `stdev`; KataGomo `gom` lines
  `MESSAGE Visits N Winrate P% …` (still `MessageEvent` with no fields; the planning's Non-goals
  say `UnparsedEvent`, but `tests/parse/rapfi_format_test.cpp` checks `MessageEvent`). Each needs
  its own todo.
- Checks: ctest `cand1_*`, `gomo7_*`, `gtp7_*`.

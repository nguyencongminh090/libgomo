---
id: DOC-02-developer-guide-prose
type: todo
size: M
status: done
owner: claude
created: 2026-11-22
supersedes: []
links: []
planning:
---
# Developer guide, pass 2: prose, structure inside sections, and fact re-check

## Scope
- Revise the sentences of `docs/guide/developer-guide.tex` in the order of note `craft-editing-and-revision`: paragraphs and sentences first, then consistency, then copy edit.
- Use skill `writing-clearly-and-concisely`: active voice, positive form, concrete words, cut needless words; remove AI-sounding patterns (`signs-of-ai-writing.md`). Read the skill's README files before using it.
- Check each section against its Diataxis type (skill `documentation`, Step 4 validation): no explanation inside how-to steps, no advice inside reference, one purpose per subsection. Fix headings so the first sentence under each says what the section is for.
- One term per concept across the whole guide (for example search, request, engine, adapter, port; `startSearch` vs "start a search"). List the chosen terms in Evidence.
- Re-check against the code what pass 1 did not: capability matrix cells (per adapter), parser input shapes and examples, ctest map rows, and the Practical rules list. Fix what is stale.
- Keep LaTeX output clean: no new overfull boxes, `latexmk` log without undefined references. Rebuild `docs/guide/developer-guide.pdf` at the end.

## Don't
- No restructuring of parts or moving sections (done in DOC-01). No figure redraws (DOC-03). No change to `include/`, `src/`, `tests/`.
- Do not change the meaning of a contract or a limitation while rewording it; when a sentence cannot be made clearer without changing its meaning, ask.
- Do not restyle the LaTeX preamble.

## Traps
- Labels `sec:*` are referenced; renaming a heading must not break them.
- `\code{}` text containing `_`, `%`, `&` needs escaping (a missing `\_` broke the build once in pass 1).
- Do not run an LLM-style rewrite over listings: code stays byte-identical unless a fact changed; a changed listing is compiled again (the first one runs against `pbrain-rapfi`).
- Rewording is the step where small factual drift happens; compare the changed paragraphs with `git diff -w` before committing.

## Done-when
Acceptance in `docs/features.json` (`"todo": "DOC-02"`).

## Evidence
Two commits: 33df804 (stale facts only), then the prose pass (so `git diff -w` of each is readable).

- build: `latexmk -pdf -interaction=nonstopmode -outdir=/tmp/pygomo-guide ...` rc 0; grep
  `undefined|Overfull` in the log: no match (baseline before the pass also had none). PDF copied
  to `docs/guide/developer-guide.pdf`.
- listings: every `lstlisting` body extracted from 51b6a50 and from the result with awk; `cmp`
  identical (244 lines). No listing line changed. First listing rebuilt out of tree with the
  tutorial's CMakeLists (add_subdirectory) and run on Rapfi `build/pbrain-rapfi`: `depth 16`,
  `best move 8,5`, exit 0.
- terms (one per concept; rejected variants grepped on newline-joined text from `\begin{document}`,
  0 hits each):
  - engine = the external program; adapter = GomocupEngine / GtpEngine / KataGomoEngine;
    port = EnginePort; decorator = DeadlineEngine. Rejected: "wrapped engine", "wrap the engine",
    "wrap that engine", "wrap it in" (they named the adapter or port "engine").
  - Gomocup (first mention: "Gomocup, also called Piskvork"). Rejected: "Gomocup/Piskvork".
  - `gom` mode. Rejected: "front end".
  - UK spelling (organised, behaviour, serialise, colours). Rejected: organiz, behavior, serializ.
  - search = one startSearch up to its terminal event; request = what startSearch or setRules
    asks for (kept as is, no variant found).
- facts (per cell, against src/ and tests/CMakeLists.txt):
  - capability matrix: Board (Gomocup and KataGomo reject width != height; GTP: formatGtpVertex
    fails above 625 columns -> Unsupported; cell was "any rectangle with a GTP vertex", now "up
    to 625 columns"); board rules all three (ruleId / basicRule / ruleCommand); Swap1/Swap2
    freestyle only on Gomocup (rule 5/6), `supports` needs FreeOpen on GTP and KataGomo;
    maxMoves all; turnTime (timeout_turn / maxTime x.yyy / timeout_turn); matchTime and maxDepth
    Unsupported on GTP and KataGomo; maxNodes (max_node / maxVisits / maxVisits); stop (YXSTOP
    while a search runs / no-op / no-op); ERROR (MessageEvent / `?` reply / terminal
    ErrorEvent); streaming and swap/pass/resign rows match onLine. All cells right except Board.
  - wire listings: Gomocup owner 1/2 relative to side to move, BEGIN on empty board; GTP ids,
    `RENJU`, `maxTime 1.000`, `interval 20` for 200 ms; KataGomo BOARD always sent, OK ignored.
  - parsers: fixed "durations accept ms/s/min/h" (only Rapfi `Time`; UCI-like `tm` is plain ms),
    "coordinates are checked by the adapter, not the parser" (GTP vertex functions check the
    board), "(x,y) for boards wider than 625" (parser reads it, adapter never sends it). Added the
    missing GTP error endings (unreadable move, no final play, reply without id). Other shapes
    (UCI-like first token, Rapfi normal/brief/MCTS/multi-PV/Bestline, K/M/G/T, +M/-M, header
    9-digit id, analyze first entry) match.
  - ctest map: all 64 entries of `ctest -N` match the rows; intro fixed (smoke has no doctest,
    deadline_adapter and live tests are not per module).
  - Practical rules: match; sendRaw bullet now points to the GTP limitation. Contract item 8
    regained "usually" from `engine_port.hpp`.
  - Found while checking: an id-less reply to `GtpEngine::sendRaw` ends a running (or the next)
    search. Not fixed (no src change); Known limitations bullet + todo GTP-06.
  - `ctest --preset default`: 100% of 64 passed (4 live skipped); check-core-deps OK;
    check-ctest-coverage OK.
- types (skill documentation, Step 4): no subsection moved or split. First sentences added where
  a subsection opened with a table, listing or list (Repository layout, Presets, Data flow,
  Practical rules, Fakes, Position, Search events, Errors, ProcessTransport, Capability matrix,
  Blocking search, DeadlineEngine, Decisions and bugs, A session, Known limitations). Reference
  DeadlineEngine no longer repeats the rationale; it links to Section Why deadlines. How-to
  "Points to notice" rewritten as instructions.
- DOC-01 checks re-run: five parts and section order unchanged, every `\label` kept, "Known
  limitations" / "never run against a real engine" still defined once.

## Links
`docs/guide/developer-guide.tex`, `.claude/skills/writing-clearly-and-concisely/`, `.claude/skills/documentation/`, SKILLS_TREE `knowledge/documents/items/craft-editing-and-revision.md`, `craft-consistency-and-style-guides.md`, `craft-reducing-ai-sounding-prose.md`; previous pass: DOC-01

---
id: DOC-03-developer-guide-figures
type: todo
size: M
status: done
owner: claude
created: 2026-11-22
supersedes: []
links: []
planning: 
---
# Developer guide, pass 3: the three TikZ figures

## Scope
- Revise the three figures of `docs/guide/developer-guide.tex`:
  1. module dependencies (Section Architecture, "Modules and dependencies");
  2. data flow of one search (sequence diagram, same section);
  3. DeadlineEngine timeline (Reference, Section Engine helpers).
- Check each figure against the code before drawing: arrows of figure 1 against
  `target_link_libraries` in the root `CMakeLists.txt`; messages of figure 2 against
  `EnginePort::startSearch`, `Transport::writeLine`, `TransportListener::onLine`,
  `EventSink::onEvent`; events and times of figure 3 against `src/engine/deadline_engine.cpp`.
  Fix what is wrong or missing.
- Make each figure a captioned, labelled `figure` float (`[tbp]`, `\caption` written first, then
  `\label{fig:*}`) and refer to it from the text with `\ref`; the sentence that introduces it stays.
- One visual style across the three: the same fonts, arrow tips, line widths and colours per
  layer (core, adapters, parse/transport, engine/process), defined once with `\tikzset` in the
  preamble rather than per picture.
- Readable in print and in black and white: no meaning carried by colour alone, text at least
  `\scriptsize`, nothing outside the text width.
- Rebuild `docs/guide/developer-guide.pdf` at the end.

## Don't
- No change to prose outside the sentences that introduce or refer to a figure (DOC-02 is done).
- No new figures and no figure removed without the owner. No change to `include/`, `src/`,
  `tests/`, and no listing changed.
- No new LaTeX package without a reason written in Evidence (`caption` or `float` are not needed
  for `[tbp]` floats).

## Traps
- Figure 1 today: `engine` also links `Threads` (only `transport` says so), and `process` is
  POSIX-only (built only when `UNIX`); decide whether the figure shows both, and say it in the
  caption if not.
- Figure 2 shows a Gomocup search (`"MESSAGE depth 5 ..."`, `"7,8"`); keep the strings in the
  protocol's real shape (the parser needs `depth` as first token for the UCI-like form).
- Figure 3: the deadline is `turnTime + grace`, then `stop()`, then one more `grace` before
  `ErrorEvent{"engine did not answer"}`; only a search with `turnTime > 0` has one.
- Floats move: text must not say "below" or "the diagram follows" once the figure floats; use
  `Figure~\ref{fig:*}`.
- Labels `sec:*` and the DOC-01 / DOC-02 checks (five parts, "never run against a real engine"
  once, no Overfull box) must still pass.
- `\code{}` text with `_`, `%`, `&` needs escaping; TikZ node text inside `\code` too.

## Done-when
Acceptance in `docs/features.json` (`"todo": "DOC-03"`).

## Evidence
- Figure 1 (`fig:modules`) against root `CMakeLists.txt` `target_link_libraries`: transport ->
  Threads; parse -> core; engine -> core, Threads; gomocup/gtp/katagomo -> core, parse,
  transport (one arrow per target from the dashed adapter box, said in the caption); process ->
  transport; core links nothing. Fixed: engine -> Threads was missing (only transport had a
  "links Threads only" note); Threads is now one dotted external node with both arrows.
  `process` carries the note "built only if UNIX" (`if(UNIX)`).
- Figure 2 (`fig:search-flow`) against `GomocupEngine::startSearch/onLine`,
  `Transport::writeLine`, `TransportListener::onLine`, `EventSink::onEvent`,
  `parseGomocupLine`: startSearch(pos, limits, sink) -> writeLine once per line (START n,
  INFO ..., BEGIN or BOARD ... DONE; was one "writeLine(...)") -> returns at once after the
  writes; `MESSAGE depth 5 ev 12` (UCI-like, `depth` first, same as the parser test) -> onLine on
  the reader thread -> parse -> onEvent(InfoEvent); `7,8` -> onLine -> parse and the
  contains/occupied check (added) -> onEvent(BestMoveEvent), terminal. Lifelines now name the
  real classes (GomocupEngine, ProcessTransport); Application and the engine program are dotted
  (outside PyGomo), "engine" = external program per the DOC-02 terms.
- Figure 3 (`fig:deadline`) against `src/engine/deadline_engine.cpp`: deadline set only when
  `turnTime > 0` (line 44) at `turnTime + grace`; watchdog calls `inner_.stop()` and re-arms
  `now + grace` (98-104); then `ErrorEvent{"engine did not answer"}`, `detached_` (106-111);
  `startSearch` returns Busy while detached (39); the late terminal clears it and is dropped
  (70-72). Fixed: third tick said "+grace", now "turnTime + 2 grace"; added the end tick "late
  terminal event / dropped". Caption covers the paths not drawn: no deadline without turnTime,
  and a terminal event before the error (even after stop) ends the search normally.
- Floats: three `figure[tbp]`, `\centering`, caption below the picture, `\caption` then
  `\label{fig:modules|search-flow|deadline}`; each referenced once by `Figure~\ref` in the
  introducing sentence ("The diagram follows" and "The timeline ...:" reworded).
- Style: one preamble block (`\colorlet` layer shades + `\tikzset` `pg *` styles); one arrow tip
  (Stealth 2 mm, 0.8 pt); fonts: `\small` boxes, `\scriptsize` labels and notes (smallest).
  Greyscale read: `docs/reports/DOC-03/guide-gray-p{7,8,20}.png` (pdftoppm -gray). Shades are
  close in grey, but every box names its module and the caption says the shade only repeats the
  layer; external boxes differ by dotted border, adapters by the labelled dashed group.
- No package added (tikz libraries already loaded: arrows.meta, positioning, fit, calc).
- Build: `latexmk` rc 0, log has no `undefined` and no `Overfull`; five `\part`, "never run
  against a real engine" once. New source lines within 100 columns (lines 409-410 over 100 are
  pre-existing prose). `docs/guide/developer-guide.pdf` copied from that build.

## Links
`docs/guide/developer-guide.tex`, root `CMakeLists.txt`, `src/engine/deadline_engine.cpp`,
`include/pygomo/core/engine_port.hpp`, `include/pygomo/transport/transport.hpp`; SKILLS_TREE
`knowledge/documents/items/tex-tables-and-figures.md`, `craft-figures-tables-captions.md`,
`fmt-diagrams-as-code.md`; previous passes: DOC-01, DOC-02

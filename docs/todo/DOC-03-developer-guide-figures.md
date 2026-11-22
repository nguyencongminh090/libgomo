---
id: DOC-03-developer-guide-figures
type: todo
size: M
status: backlog
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
Filled in before marking done: per-figure check against the code, packages added (if any), build
log result, before/after page images if useful (`docs/reports/`).

## Links
`docs/guide/developer-guide.tex`, root `CMakeLists.txt`, `src/engine/deadline_engine.cpp`,
`include/pygomo/core/engine_port.hpp`, `include/pygomo/transport/transport.hpp`; SKILLS_TREE
`knowledge/documents/items/tex-tables-and-figures.md`, `craft-figures-tables-captions.md`,
`fmt-diagrams-as-code.md`; previous passes: DOC-01, DOC-02

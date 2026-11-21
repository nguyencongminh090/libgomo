---
id: DOC-01-developer-guide-restructure
type: todo
size: M
status: done
owner: claude
created: 2026-11-22
supersedes: []
links: []
planning:
---
# Developer guide, pass 1: restructure the content by documentation type

## Scope
- Reorganise `docs/guide/developer-guide.tex` by Diataxis type (skill `documentation`, note `tech-diataxis-types`):
  1. Introduction and Getting started (tutorial: build, then run a first search).
  2. Explanation: architecture, enforced rules, why the deadline lives in `DeadlineEngine`.
  3. How-to: search with a deadline, asynchronous sink, add an adapter, a transport, an event, a test.
  4. Reference: core types, `EnginePort` contract and errors, transports, adapters and capability matrix, parsers, helpers, ctest map.
  5. Contributing: coding style, how work is organised, known limitations (stated once).
- Move and split sections; add what is missing (Getting started); remove duplicated facts (limitations repeated in Status, adapters and Known limitations).
- Check each moved fact against the code while moving it; fix what is stale.
- Pass 1 only. Prose and style are pass 2, the three TikZ figures are pass 3.

## Don't
- No change to `include/`, `src/`, `tests/`. No rewording for style (pass 2), no redrawing figures (pass 3).
- Do not edit `third_party/`. Do not add LaTeX packages without a reason.

## Traps
- Section labels (`sec:layers`, `sec:port`, `sec:testing`, `sec:process`) are referenced; keep them or update every `\ref`.
- The two listings in "Using the library" were compiled once against fake engines; if they move unchanged, they stay valid. If edited, compile them again.
- `docs/guide/developer-guide.pdf` is committed; rebuild it at the end of the pass.

## Done-when
Acceptance in `docs/features.json` (`"todo": "DOC-01"`).

## Evidence
- build: `latexmk -pdf` rc 0, no undefined reference; `docs/guide/developer-guide.pdf` rebuilt.
- structure: five parts (Getting started, Explanation, How-to guides, Reference, Contributing); sections moved as whole blocks, so no text was lost.
- added: "How this guide is organised", "Your first search", "Why deadlines live in a decorator"; limitations stated once (Section Known limitations), Status trimmed.
- facts checked against code: EnginePort/EngineError/SearchEvent/Limits/Rules/ProcessOptions/DeadlineEngine::Options, presets, ctest group names, AUD ids, fix-log entry. Not re-checked this pass: capability matrix cells, parser shapes (pass 2 reads them again).
- tutorial: built the listing out of tree with add_subdirectory and ran it on real pbrain-rapfi: "depth 16", "best move 8,5", exit 0.
- green: ctest 64/64 (4 live skipped), check-core-deps OK, check-ctest-coverage OK.

## Links
`docs/guide/developer-guide.tex`, `.claude/skills/documentation/`, SKILLS_TREE `knowledge/documents/items/tech-diataxis-types.md`, `craft-editing-and-revision.md`

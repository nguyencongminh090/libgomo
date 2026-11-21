---
id: DOC-02-developer-guide-prose
type: todo
size: M
status: backlog
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
Filled in before marking done: the term list, the matrix and parser checks, the build log result.

## Links
`docs/guide/developer-guide.tex`, `.claude/skills/writing-clearly-and-concisely/`, `.claude/skills/documentation/`, SKILLS_TREE `knowledge/documents/items/craft-editing-and-revision.md`, `craft-consistency-and-style-guides.md`, `craft-reducing-ai-sounding-prose.md`; previous pass: DOC-01

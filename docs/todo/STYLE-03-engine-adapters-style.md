---
id: STYLE-03-engine-adapters-style
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: code-style-normalization
---
# Code style for engine, gomocup, gtp and katagomo

## Scope
- Apply `.claude/rules/coding-style.md` to `include/pygomo/{engine,gomocup,gtp,katagomo}`, `src/{engine,gomocup,gtp,katagomo}`, `tests/{engine,gomocup,gtp,katagomo}`:
  commit 1 = doc comments + lines within 100 columns; commit 2 = `style: align columns` (whitespace only).
- Doc comments on adapters state thread-safety, which thread calls the sink, deadline behaviour and which `EngineError` each call returns.
- Do after STYLE-02.

## Don't
- No logic, rename, reorder or API change. Nothing outside these directories. Never touch `third_party/`.

## Traps
- `tests/gomocup/gomocup_engine_test.cpp` has the most long lines (11); scripted engine replies are protocol text, same literal rule as STYLE-02.
- Behaviour claims in comments must match the code and its tests; read before writing, do not copy from planning docs.

## Done-when
Acceptance in `docs/features.json` (`"todo": "STYLE-03"`).

## Evidence

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

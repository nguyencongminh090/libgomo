---
id: STYLE-02-parse-transport-process-style
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: code-style-normalization
---
# Code style for parse, transport and process

## Scope
- Apply `.claude/rules/coding-style.md` to `include/pygomo/{parse,transport,process}`, `src/{parse,transport,process}`, `tests/{parse,transport,process}`:
  commit 1 = doc comments + lines within 100 columns; commit 2 = `style: align columns` (whitespace only).
- Do after STYLE-01 (uses its check scripts).

## Don't
- No logic, rename, reorder or API change. Nothing outside these directories. Never touch `third_party/` or `tests/fixtures/`.

## Traps
- Parser tests hold protocol text in string literals: wrapping or aligning inside a literal changes the test input. Only split literals with adjacent-literal concatenation that keeps the bytes identical, or leave them.
- `process` comments must state blocking, signal and child-lifetime behaviour, not restate POSIX calls.

## Done-when
Acceptance in `docs/features.json` (`"todo": "STYLE-02"`).

## Evidence

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

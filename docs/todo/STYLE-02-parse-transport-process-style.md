---
id: STYLE-02-parse-transport-process-style
type: todo
size: M
status: done
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
Commit `08cf296` (doc comments + wrapped lines). No align commit: nothing in the changed blocks was misaligned (ProcessOptions and the TransportError enum were already aligned or written aligned in commit 1), so the whitespace-only diff is empty by construction.
- lines, docs: `check-line-length.sh` and `check-doc-comments.sh` on the STYLE-02 dirs exit 0 (before: 21 long lines, 17 undocumented declarations).
- Parser tests: only list initialisers and statements wrapped; no string literal was split or changed, except one in process_transport_test.cpp split into adjacent literals with identical bytes.
- green: build, `ctest --preset default` 62/62 (4 live skipped), `check-core-deps.sh` OK.

## Links
`features/code-style-normalization/planning.md`, `.claude/rules/coding-style.md`

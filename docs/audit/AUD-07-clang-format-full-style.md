---
id: AUD-07-clang-format-full-style
type: audit
size: M
status: active
owner: claude
created: 2026-11-23
supersedes: []
links: [FMT-03-full-style-fences, AUD-06-clang-format-measure]
---
# clang-format full style (S2): measured diff and decision

## Context

Phase 2 of planning `clang-format` asks for a full-style config (S2) and `clang-format off`/`on`
fences (O2) where code must stay as written. Measured first, with `clang-format --style=file:X F | diff F -`
over `include/ src/ tests/` (clang-format 21.1.8, tree unchanged).

## Decision

| Candidate config | include/ | src/ | tests/ |
|---|---|---|---|
| Plain LLVM + 4 indent, 100 cols, left pointers, alignment, `SortIncludes` on | 228 | 669 | 425 |
| Phase 1 config (layout options kept) + `SortIncludes: CaseSensitive` | 0 | 2 | 2 |

Plain LLVM rewrites about 1300 lines (short `if`/loops/functions, penalties, wrapping), which is churn
with no gain. Chosen: keep the layout options of phase 1 (they are the style the code already has) and
make the rest explicit: `IncludeBlocks: Preserve`, `SortIncludes: CaseSensitive` (sorts inside each
block). The reformat touched two include lines (`<cstring>` and `<optional>` moved into order).

No `// clang-format off` fence was needed: nothing in the tree resists the config. Fences stay the
tool for a future block that must keep its layout, each with a one-line reason.

## Status

Active. Input to FMT-04.

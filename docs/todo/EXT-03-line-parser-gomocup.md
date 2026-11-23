---
id: EXT-03-line-parser-gomocup
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# GomocupEngine: pluggable LineParser and chainParsers

## Scope
Add `LineParser = std::function<SearchEvent(std::string_view)>` and `chainParsers(custom, fallback)` (the custom parser runs first; an `UnparsedEvent` result falls through to the default). `GomocupEngine` takes an optional parser in its constructor; default is `parseGomocupLine`.

## Don't
Do not change `parseGomocupLine` signature. Do not drop the ERROR-to-MessageEvent rule (GOMO-04) or the illegal-move check.

## Traps
Both hooks run on the reader thread, never under the adapter lock. A custom parser must not turn a terminal line into a non-terminal one silently: document it.

## Done-when
Features `EXT-03-*` in `docs/features.json` (`harness-track EXT-03`).

## Evidence
2026-11-23. `parse::LineParser` and `parse::chainParsers` in `parse/line_parser.hpp` (header only); `GomocupEngine(Transport&, LineParser = {})`, empty means `parseGomocupLine`. The parser runs on the reader thread before the lock; the ERROR-to-Message rule and illegal-move check apply to its result. Terminal caveat is in the `LineParser` doc comment. Tests `ext03_*`; suite 88/88.

## Links
`features/extensible-engine-commands/planning.md`

---
id: GTP-06-raw-reply-ends-search
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-22
supersedes: []
links: []
planning: 
---
# GtpEngine: the id-less reply to a raw command ends a search

## Scope
- Reported during DOC-02 (found by reading the code, not reproduced by a test yet).
- `GtpEngine::sendRaw` writes the line as is, without an id. When its `=` reply is read while a
  search runs (also a search started right after the raw command: `startSearch` clears
  `inReply_`), `onReply` reaches `if (!id) return finish(ErrorEvent{"engine reply carries no
  id"})` and ends the search. The guide's Practical rules recommend `sendRaw` for
  `kata-set-param` keys, which is exactly this case.
- Reproduce with a fake-transport test first, then choose a fix (for example give raw commands
  an id outside the search's id range, or ignore id-less replies while a search's own commands
  all carry ids). The choice changes protocol behaviour: record it in an audit.

## Don't
- No change to the `EnginePort` contract ("sendRaw promises nothing").
- Do not touch the Gomocup or KataGomo adapters.

## Traps
- An id-less `?` reply is also the only way some engines report a broken setup line; do not hide
  real failures of the search's own commands.
- After the fix, update `docs/guide/developer-guide.tex` (Known limitations bullet "GTP: a raw
  command can end a search").

## Done-when
Acceptance in `docs/features.json` (`"todo": "GTP-06"`), to be added by the owner.

## Evidence
Filled in before marking done.

## Links
`src/gtp/gtp_engine.cpp` (`onReply`, `startSearch`), `docs/guide/developer-guide.tex`, DOC-02

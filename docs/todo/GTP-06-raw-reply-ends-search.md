---
id: GTP-06-raw-reply-ends-search
type: todo
size: M
status: done
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
Acceptance in `docs/features.json` (`"todo": "GTP-06"`); features approved by the owner on 2026-11-22.

## Evidence
- Reproduced first: 3 of 5 initial `gtp6_raw` cases failed before any fix (raw-before, raw-during, header read before the search).
- First fix (pending-raw counter) was rejected by the reviewer agent: a raw line sent during setup let an id-less setup failure pass as the raw reply (the Trap). Replaced by a write-order reply queue (AUD-04, fix-log GTP-06-01); `gtp6_raw` now has 9 cases, including that interleaving, a raw line with its own id, raw after genmove, and a failed write. Raw-during now uses an order a real engine can produce.
- `ctest --preset default`: 65/65 pass (`gtp6_raw` registered); `ctest --preset tsan`: 65/65 pass; `ctest --preset tsan -R gtp --repeat until-fail:30` pass.
- `scripts/check-core-deps.sh` OK, `scripts/check-doc-comments.sh` OK, `harness-check.mjs` 0 findings.
- Guide: Practical rules, GTP reply framing and Known limitations updated; `grep "raw command can end"` finds nothing; PDF rebuilt with latexmk rc 0, no overfull or undefined warnings (same as before).

## Links
`src/gtp/gtp_engine.cpp` (`onReply`, `startSearch`), `docs/guide/developer-guide.tex`, DOC-02

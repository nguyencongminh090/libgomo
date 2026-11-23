---
id: EXT-04-line-parser-gtp-katagomo
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# GtpEngine and KataGomoEngine: pluggable LineParser

## Scope
Same hook for `GtpEngine` (called per line of a reply body; reply matching stays in the adapter) and `KataGomoEngine`. Defaults unchanged.

## Don't
Do not change reply attribution (AUD-04). No change to the analyze parser signature.

## Traps
GTP: a custom parser must not see the `=`/`?` header or the blank line that ends a reply.

## Done-when
Features `EXT-04-*` in `docs/features.json` (`harness-track EXT-04`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

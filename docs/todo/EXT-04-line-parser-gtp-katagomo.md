---
id: EXT-04-line-parser-gtp-katagomo
type: todo
size: M
status: done
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
2026-11-23, agy (gemini-4-neon-medium), reviewed and fixed by Claude. `ctest -R '^ext04_'` pass (ext04_gtp, ext04_kata, plus ext02_kata from the EXT-02 note); full suite 96/96 on master. Review fix: GTP showed stale reply bodies of an earlier search to the new search's parser; now skipped, regression case `ext04_gtp: the body of a stale reply...` (fails without the fix). GTP rule: custom parser first on reply body and free-text lines during a search; non-terminal result consumes the line; Unparsed or terminal falls back (only a reply ends a search).

## Links
`features/extensible-engine-commands/planning.md`

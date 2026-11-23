---
id: EXT-08-request-encoder-gtp-katagomo
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# GtpEngine and KataGomoEngine: RequestEncoder

## Scope
Same hook and default-script builders for `GtpEngine` and `KataGomoEngine`.

## Don't
No change to default lines. No `kata-set-rule` mapping (out of scope).

## Traps
GTP builds one command per line and matches replies by count (AUD-04): an encoder that adds commands changes the reply count; compute it from the returned lines.

## Done-when
Features `EXT-08-*` in `docs/features.json` (`harness-track EXT-08`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

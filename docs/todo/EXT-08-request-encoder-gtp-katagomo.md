---
id: EXT-08-request-encoder-gtp-katagomo
type: todo
size: M
status: done
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
2026-11-23, codex (gpt-6.2-sol xhigh). `ctest -R '^ext08_'` 9/9; full suite 109/109 on master after merge (live skipped). GTP encoder returns commands without ids; the adapter numbers them and computes the reply count from the list (AUD-04); the last command requests the move and analyze mode is chosen by its prefix (`kata-genmove_analyze`). Empty list or a blank/comment line is Unsupported. Default refusals (matchTime, maxDepth, non-square for KataGomo) now come after the NotReady/Busy checks.

## Links
`features/extensible-engine-commands/planning.md`

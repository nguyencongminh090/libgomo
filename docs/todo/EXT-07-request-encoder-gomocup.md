---
id: EXT-07-request-encoder-gomocup
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# GomocupEngine: RequestEncoder and pygomo::encode

## Scope
Add `SearchRequest{rules, position, limits}`, `RequestEncoder`, and `pygomo::encode::defaultGomocupRequest` plus `boardBlock(command, moves, extra)`. `GomocupEngine` takes an optional encoder; default is the current script. With no encoder, `Rules::extensions` or a non-default layout gives `Unsupported` and nothing is sent. Test with a fake Rapfi encoder that sends `INFO RULE 8`, `INFO WALL`, `INFO TORUS 1`, `INFO YXPORTAL`, and `YXBOARD` with colour 3 for walls.

## Don't
No edit to existing Gomocup tests. The encoder must not call back into the adapter.

## Traps
The encoder runs under the adapter lock today (lines are built there): keep it pure. Build errors (`Unsupported`) must send nothing.

## Done-when
Features `EXT-07-*` in `docs/features.json` (`harness-track EXT-07`).

## Evidence
2026-11-23, Opus subagent. `ctest -R '^ext07_'` 4/4; full suite 100/100 on master after merge (live skipped); check-core-deps OK; no existing test edited. Extra public alias `EncodedRequest` (keeps `RequestEncoder` on one documented line). The square check moved into the default encoder, so a non-square board while Busy/closed now gives Busy/NotReady first. Not done (scope): the illegal-move check does not reject a move on a blocked cell.

## Links
`features/extensible-engine-commands/planning.md`

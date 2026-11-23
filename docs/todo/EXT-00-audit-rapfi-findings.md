---
id: EXT-00-audit-rapfi-findings
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# Audit: Rapfi extension findings (VC2, YXANALZ, WALL, Torus, Portal, Caro, BOARD, STOP)

## Scope
Write `docs/audit/AUD-08-rapfi-extension-findings.md` from the Findings table of the planning: for each of `INFO VCTHREAD`, `YXANALZ`, `INFO RULE 8`, `INFO WALL`, `INFO TORUS`, `INFO YXPORTAL`, `RECTSTART`, `BOARD`/`YXBOARD`/`SWAP2BOARD`/`TURN`/`YXBLOCK`, `YXSTOP` record the line format, the reply, and the source file and line in Rapfi V2/V3. Also read `ABOUT` and `YXSHOWINFO` and record how a reply to a one-shot command ends (input for the `Until` completion of EXT-10).

## Don't
Keep no code. Do not change the planning (it is frozen); a correction is a new audit that supersedes.

## Traps
Rapfi paths are outside the repo (handoff lists them). Line numbers drift: name the function too.

## Done-when
Features `EXT-00-*` in `docs/features.json` (`harness-track EXT-00`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

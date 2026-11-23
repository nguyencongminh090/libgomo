---
id: EXT-00-audit-rapfi-findings
type: todo
size: M
status: done
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
2026-11-23. `docs/audit/AUD-08-rapfi-extension-findings.md`, read from Rapfi V2 `d14b636` and V3
`Rapfi_V3_PTR/dev` (not a git repo; identified by mtime and sha256 in the audit; `stable` has no Caro or
rect support). Until: Rapfi prints no terminator; replies are a fixed line count (ABOUT 1, YXSHOWINFO 3),
`OK`/one `ERROR`, or nothing (INFO, YXBOARD, YXBLOCK). The audit lists corrections to the frozen
planning (V3 has no YXANALZ and ignores VCTHREAD; V3 clears `thinking` after printing the move, so a
command sent right after the terminal line can still be dropped; checked at V3 gomocup.cpp:123-133,1128).
- harness-check: 0 findings. Keyword grep loop: no MISSING.

## Links
`features/extensible-engine-commands/planning.md`

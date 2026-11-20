---
id: GOMO-03-lowercase-info-keys
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: engine-protocol-port
---
# GomocupEngine: `INFO` upper case, keys lower case

## Scope
- `GomocupEngine::startSearch` sends `INFO rule N`, `INFO max_moves N`, `INFO timeout_turn MS`,
  `INFO timeout_match MS`, `INFO max_depth N`, `INFO max_node N`. Commands (`START`, `BOARD`, `DONE`, `YXSTOP`)
  stay upper case. Owner's rule: `INFO` upper, key lower.
- Why: Gomocup managers send lower-case keys and KataGomo `gom` mode compares `timeout_turn` case-sensitively
  (`gomprotocol.cpp`), so the upper-case form is silently ignored there. Rapfi accepts both: it upper-cases the key
  (`command/gomocup.cpp:191`) and the command (`:956-957`). Source reading only until the live run below.
- Update the four test lines and two feature descriptions that name the upper-case text (owner-approved by this
  rule, listed in Evidence); change nothing else in any test or feature.
- Re-run the live Rapfi tests with the lower-case form.

## Don't
- No new options, dialect switch or adapter (KataGomo is a separate adapter, todo KATA-01).
- Do not touch the parser, core or transport.

## Traps
- `INFO RULE` has no effect on KataGomo at all (rules there are separate commands); this todo does not fix that.
- Keys Rapfi adds beyond the Gomocup spec (`max_depth`, `max_node`, `max_moves`) are Rapfi-only; other engines may ignore them.

## Done-when
Acceptance lives in `docs/features.json` (`"todo": "GOMO-03"`); `harness-track GOMO-03` lists them.
(Features approved by the owner on 2026-11-20 and written to `docs/features.json`; all `passes: false` until run.)

## Approved test and feature text changes (owner, 2026-11-20)
- `tests/gomocup/gomocup_engine_test.cpp` lines 72, 92, 93, 106: `INFO RULE 0`, `INFO TIMEOUT_TURN 5000`, `INFO MAX_DEPTH 12`, `INFO RULE 4`, `INFO MAX_MOVES 200`, `INFO RULE 6` become `INFO rule 0`, `INFO timeout_turn 5000`, `INFO max_depth 12`, `INFO rule 4`, `INFO max_moves 200`, `INFO rule 6`.
- Descriptions (not `checked_by`, not `passes`) of `GOMO-01-rules` (`INFO RULE 4` -> `INFO rule 4`) and `GOMO-01-sequence` (`INFO RULE` -> `INFO rule`).
- Nothing else in any test or feature may change.

## Evidence
(fill in before marking done)

## Links
- `docs/todo/GOMO-01-gomocup-adapter.md`, `docs/todo/GOMO-02-real-engine-rapfi.md`,
  `docs/notes/2026-11-20-kata-gomprotocol-findings.md`.

# Spike: CommandOptions::force against a real Rapfi V2 and V3 (2026-11-23)

Spike, not kept code. Answers the handoff item "Command/force against a real Rapfi V2 (YXANALZ)
and V3". Background: docs/audit/AUD-08 "Corrections to the planning" (V3 drop race).

## Setup
Throwaway program: `GomocupEngine` over `ProcessTransport`, a 60 s search on the 3-move 15x15
position, then after 500 ms `execute(command, sink, {force = true})`. Each round checks that the
old search ends with a move, the command answers, and a new 500 ms search still answers.

## Results
- V2 (`Lab/Rapfi_V2/rapfi/Rapfi/build/pbrain-rapfi`), 5 rounds each:
  - Search (`INFO timeout_turn 500` + `BOARD` block): 5/5.
  - Until (`ABOUT`, done on the first event): 5/5.
  - Search `INFO timeout_turn 500`, `YXANALZ`, `6,6`, `9,9`, `DONE`: 5/5; the move was always
    9,9 (the whitelist holds).
  - From the forced stop to the command's move: about 470 ms.
- V3 dev (`Lab/Rapfi_V3_PTR/dev/rapfi/gomoku-portal-ui-distribute/pbrain-rapfi`), 15 rounds each:
  - Search (`BOARD`): 15/15. Until (`ABOUT`): 15/15.
  - The AUD-08 drop race was not seen in 30 rounds. It is not ruled out: it needs the command to
    arrive between the move line and `thinking` being cleared.
- `ABOUT` parses as `UnparsedEvent`, so an Until on it must accept that event.

## Not covered
V3 stable; YXANALZ with no valid move under force (expected: ERROR is not terminal, use Until);
force of a None command (no reply, a drop cannot be seen from the sink).

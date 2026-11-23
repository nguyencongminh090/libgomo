---
id: LIVE-01-force-live-test
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [EXT-10-command-port-force]
---
# Live test: Command/force against Rapfi V2 and V3

## Scope
Turn the spike `docs/reports/rapfi-force-live-spike-2026-11-23.md` into a permanent live test `ext10_live` in `tests/live/` (same pattern as `ext09_live`: skipped without the env var). Cases: V2 Search (`BOARD`), Until (`ABOUT`, accepts `UnparsedEvent`), and `YXANALZ` (move is the whitelisted one); V3 Search and Until. Each case: force during a running search, the old search ends with a move, the command answers, a new short search still answers. A few rounds per case, not 15.

## Don't
Do not keep the spike program. Do not change library code. Do not fail when a binary is missing (skip).

## Traps
V2 needs `PYGOMO_RAPFI_V2`, V3 needs `PYGOMO_RAPFI_V3` (cwd = binary dir for config.toml). The AUD-08 V3 drop race is timing dependent: the test must not flake, so it asserts the outcomes seen in the spike, not the race itself. ERROR is not terminal for `YXANALZ` with no valid move (use Until).

## Done-when
Features `LIVE-01-*` in `docs/features.json` (`harness-track LIVE-01`).

## Evidence
(none yet)

## Links
`docs/reports/rapfi-force-live-spike-2026-11-23.md`, `docs/audit/AUD-08*`

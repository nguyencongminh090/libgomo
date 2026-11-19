---
id: GOMO-02-real-engine-rapfi
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# GomocupEngine against the real Rapfi

## Scope
- Capture real Rapfi transcripts (read-only use of the owner's build outside the repo) into
  `tests/fixtures/gomocup/`, each with a header: engine version, command, config.
- Extend `parseGomocupLine` to Rapfi's normal/brief message formats (source: `search/searchoutput.cpp`,
  `core/utils.cpp` text formatters: K/M/G/T nodes, ms/s/min/h times, `H8` labels).
- Live tests (`PYGOMO_RAPFI` env var, skipped when unset): search, stop, close.

## Don't
- Do not modify or build anything in the Rapfi tree. No engine code or binary in this repo.
- No core change (message events are a separate L decision).

## Traps
- Rapfi may ponder after a move; ponder lines arrive outside a search. Check against fixtures.
- Config `coord_conversion_mode` flips output coordinates; fixtures use `none`.

## Done-when
`docs/features.json`, `"todo": "GOMO-02"`.

## Evidence
2026-11-19, Rapfi 0.44.01 (owner build, read-only use; nothing built or changed in its tree):
- Source re-read before running: `search/searchoutput.cpp`, `core/utils.cpp`, `core/iohelper.*`, `command/gomocup.cpp`, `config.cpp`.
- Fixtures: 3 transcripts in `tests/fixtures/gomocup/` (default config, partial `message_mode = normal`, partial `ucilike`; partial configs have no model section so Eval stays 0). `gomo2_fixtures`, `gomo2_parse` passed.
- Live (`PYGOMO_RAPFI` set): `gomo2_live`, `gomo2_stop` (0.5 s, YXSTOP works, no retry needed), `gomo2_close` passed; 5 repeats clean; whole suite 31/31 also under tsan; no Rapfi process left. Without the variable the three tests report Skipped and ctest exits 0.
- Test change to report: PARSE-01's `parse_unknown` asserted that `MESSAGE Depth 5 | Eval 3 | Time 1s | H8` stays Unparsed, which pinned the limitation this todo (owner-approved) removes. That one line now uses `MESSAGE Depth five | Eval x | Time soon`; the real shape is covered by `gomo2_parse`. No other test or feature was weakened.
- Not covered: pondering lines arriving late (not seen in the fixtures), `coord_conversion_mode` other than none, mate scores only from the source (no real line captured).

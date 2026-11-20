---
id: TEST-01-ctest-covers-every-doctest-case
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# Check that ctest runs every doctest case

## Scope
- A script (e.g. `scripts/check-ctest-coverage.sh`, bash or Node, never python) that, for every test binary registered in ctest, lists its doctest cases (`<binary> -ltc`) and the `-tc=` filters ctest registers for it (`ctest --preset default -N -V`), and fails naming each case that no registered entry runs. An entry with no `-tc=` filter covers the whole binary.
- Register the script as a ctest test (or add it to the Commands in `CLAUDE.md`) so `ctest --preset default` fails when a new case is not registered.
- Prove it: temporarily add a case with an unregistered prefix, the check fails naming it; remove it, the check passes.

## Don't
- Do not change how existing tests are registered or grouped.
- Do not edit or remove existing tests.

## Traps
- `-tc=` filters are doctest wildcards (`*`, `?`), possibly comma-separated; match them as wildcards, not as plain prefixes.
- `-ltc` output has doctest header/footer lines; strip them. Use `--no-version`.
- Live tests that skip without an engine are still registered; they must count as covered.
- The check needs a built tree: run it after the build, against `build/default`.

## Done-when
Acceptance in `docs/features.json` (`"todo": "TEST-01"`), proposed and owner-approved before code.

## Evidence
- Added `zz_unregistered: probe` to `tests/transport/stream_transport_test.cpp` (rebuilt): script exit 1 naming `[pygomo_transport_tests] zz_unregistered: probe`, `ctest_coverage` Failed. Reverted: exit 0, full `ctest --preset default` passes.
- Core binary: the probe is covered by `core_tests` (no -tc=), as specified.

## Links
Fix-log GTP-04-01 (`gtp4_message` built but never run by ctest), `tests/CMakeLists.txt`.

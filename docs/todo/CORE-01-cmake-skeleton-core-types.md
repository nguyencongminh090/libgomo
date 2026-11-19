---
id: CORE-01-cmake-skeleton-core-types
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# CMake skeleton and protocol-neutral core types

## Scope
- CMake project (C++23) with a `pygomo_core` library, a test target and presets (`default`).
- Core value types only, no I/O, no threads: `Coord` (0-based x, y, top-left origin), `Move`
  (coord, side), `Position` (width, height, ordered moves, side to move; rectangular boards
  allowed), `Rules` (`BoardRule`, `OpeningRule`, optional `maxMoves`), `Limits` (time, depth,
  nodes), `SearchEvent` (variant: `Info`, `BestMove`, `Swap`, `Pass`, `Error`, `Unparsed`).
- Unit tests for each type (construction, validation, equality).
- Define the build, test and smoke commands in `CLAUDE.md` and `AGENTS.md` (the harness leaves
  them as TODO).

## Don't
- No `Transport`, parser, adapter or `EventSink`: later todos.
- No protocol names, process or stream headers in the core library.
- No third-party dependency in the core; the test framework is the only allowed exception.

## Traps
- `Rules` must not copy Rapfi's merged numeric `RULE` id; keep board rule and opening rule
  separate (planning, Decision, Rules).
- Coordinates are converted only in adapters; the core never sees GTP letters or row-from-bottom.
- Test framework: doctest 2.4.11 (owner approved 2026-11-19), vendored as a single header in
  `third_party/doctest/` so the build works offline.

## Done-when
Acceptance lives in `docs/features.json`: one entry per check, `"todo": "CORE-01"`, description `WHEN <action> THEN <observable result>`, `steps`, `checked_by`, `"passes": false`. Flip `passes` only after running `checked_by`; adding or changing features is the owner's call. `harness-track CORE-01` lists them.

## Evidence
Clean rebuild (rm -rf build/default) on 2026-11-19, g++ 15.2, CMake 4.2.3, Ninja 1.13.2:

- CORE-01-build: `cmake --preset default && cmake --build --preset default` exit 0, 0 warnings in the log (warnings are errors in the preset).
- CORE-01-tests: `ctest --preset default` 2/2 passed; doctest: 22 test cases, 103 assertions, 0 failed.
- CORE-01-core-deps: `scripts/check-core-deps.sh` printed "check-core-deps: OK", exit 0. Negative test: a temporary header with `#include <thread>` and a protocol-named include made it exit 1; removed afterwards.
- CORE-01-smoke: `ctest --preset default -R smoke` passed.

## Links
- features/engine-protocol-port/planning.md

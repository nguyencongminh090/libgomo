# PyGomo

C++ framework/protocol layer connecting applications to Gomoku engines. Focus: low coupling, multi-protocol, open/extensible interfaces, clean code, parsing engine text output.

## SKILLS_TREE library

Before writing a skill-like workflow, design decision or review checklist from scratch, search the library (do not browse or grep it; never use python3):

`/run/media/ngmint/Data/Programming/Programming/SKILLS/SKILLS_TREE/tools/find "<need in plain English>"` (add `-k knowledge|skill|rule`, `-d <domain>`)

Read hits in place (knowledge notes and rules are not copied). To use a skill, copy only that item into `.claude/skills/` per the library README section 3. Starting points: `pattern-adapter`, `principle-coupling-cohesion`, `hexagonal-ports-adapters`.

## Commands

Toolchain: CMake >= 3.25, Ninja, a C++23 compiler (checked with g++ 15). Tests use doctest 2.4.11, vendored in `third_party/doctest/`.

- Build: `cmake --preset default && cmake --build --preset default` (Debug, warnings are errors)
- Test: `ctest --preset default`
- Single test: `./build/default/tests/pygomo_core_tests -tc="<test case name>"`
- Core dependency check: `scripts/check-core-deps.sh` (pygomo_core must include no I/O, thread, process or protocol header)
- Smoke: `ctest --preset default -R smoke`
- Harness check: `node scripts/harness/harness-check.mjs`
- ThreadSanitizer: `cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan`

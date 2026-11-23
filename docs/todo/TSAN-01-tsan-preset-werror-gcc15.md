---
id: TSAN-01-tsan-preset-werror-gcc15
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: 
---
# tsan preset fails to build on g++ 15 (-O2 -Werror false positives)

## Scope
Make `cmake --preset tsan && cmake --build --preset tsan` build again with warnings as errors.
Found during EXT-10 (2026-11-23); already broken on master since EXT-02:
- `src/engine/blocking_search.cpp` and `tests/gomocup/gomocup_engine_test.cpp`:
  `new_allocator.h:172: operator delete called on unallocated object [-Werror=free-nonheap-object]`
  (copy of `CustomEvent` fields, `Value` variant).
- `src/engine/deadline_engine.cpp`: `std_function.h:245 _M_manager may be used uninitialized
  [-Werror=maybe-uninitialized]` (copy of `Command`, `Until` holds a `std::function`).
With `-DPYGOMO_WARNINGS_AS_ERRORS=OFF` the tsan suite passes 118/118 with no race report.

## Don't
Do not turn warnings-as-errors off for the whole preset without an owner decision. Do not
weaken `checked_by` of EXT-10-tsan.

## Traps
Both look like known GCC 15 false positives under -O2 with sanitizers; a targeted
`-Wno-...` on the tsan preset (with a comment and the GCC bug id) is a toolchain change: owner
decides. Rewriting code only to silence the compiler must not change behaviour.

## Done-when
Owner adds features; until then: the tsan preset builds and `ctest --preset tsan` passes, which
also unblocks feature `EXT-10-tsan`.

## Evidence
(fill in before marking done)

## Links
`docs/todo/EXT-10-command-port-force.md` (Evidence), `CMakePresets.json`.

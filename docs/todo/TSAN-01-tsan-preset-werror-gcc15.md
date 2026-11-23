---
id: TSAN-01-tsan-preset-werror-gcc15
type: todo
size: M
status: done
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
2026-11-23, Opus. Owner chose `-Wno-error=free-nonheap-object -Wno-error=maybe-uninitialized` on the tsan preset only (CMakePresets.json; the reason is in the preset `description`, JSON has no comments). Measured: the warnings also appear at plain -O2 without -fsanitize (not tsan specific), all inside libstdc++ headers (variant/vector/string copies of `Value`, `std::function` copy in `Command`); GCC still prints them as warnings. No GCC bug id cited (not verified). clang 21 compiles the src files clean but doctest trips `-W#warnings` (<ciso646>), so switching compiler was not an option. Clean tsan build passes; `ctest --preset tsan` 118/118, no ThreadSanitizer report; default 118/118. TSAN-01-scope check fails on a mutated preset (an extra `-Wno-error`). Features TSAN-01-build/-scope/-suite; EXT-10-tsan now passes.

## Links
`docs/todo/EXT-10-command-port-force.md` (Evidence), `CMakePresets.json`.

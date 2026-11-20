# Fix-log

Append-only. One entry per wrong-layer bug; see `templates/fix-log.md`.

## GTP-04-01-gtp4-test-not-registered (2026-11-20, owner: claude)

### Symptom
GTP-04 features were marked passing with `ctest --preset default -R gtp`, but that command never ran the new
`gtp4_message` test case. Found while drafting GTP-05 features (`ctest -N` listed no `gtp4` entry).

### First fix
None. GTP-04 relied on `ctest -R gtp` as its `checked_by`, assuming every test case in `pygomo_gtp_tests` is
run by ctest.

### Real source
`tests/CMakeLists.txt` registers each doctest group by name filter (`-tc=gtp2_<group>*`, `-tc=gtp3_<group>*`).
A new prefix (`gtp4_`) needs its own `add_test`; without it the case is built and silently never run.

### Corrected fix
`add_test(NAME gtp4_message COMMAND pygomo_gtp_tests "-tc=gtp4_message*")` (commit d1c2377).
`ctest -R gtp` now runs 12 entries, `gtp4_message` among them, and passes.

### Check that would have caught it
Compare each test binary's `-ltc` list against the registered `-tc=` filters and fail on any case no filter
matches (done by hand on 2026-11-20: only `gtp4_message` was missing). Not yet a script; candidate for a todo.
Also: when a todo adds a test case, its evidence should show the case name in `ctest` output.

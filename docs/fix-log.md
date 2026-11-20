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

## PORT-02-01-blocking-test-sink-use-after-free (2026-11-20, owner: claude)

### Symptom
`port_blocking` hung (22+ min) under the tsan preset on 2026-11-20; passed alone. Reproduced on the first
tsan run as a TSan SEGV in the worker thread at `tests/engine/blocking_search_test.cpp:27`.

### First fix
None. Suspected from reading (overlapping ctest runs were a distraction).

### Real source
The threaded `ScriptedEngine` worker sent an `InfoEvent` after the `BestMoveEvent`. `search()` returns as soon
as the terminal event is seen and destroys its stack `CollectingSink`; the worker then called into a freed
sink (use-after-free). Test bug, not a `blocking_search.cpp` bug; the `EventSink` contract is unchanged.

### Corrected fix
The after-terminal event is sent in the sync variant only (the sink is still alive there), and the
assertions (`info.size() == 2`) still prove it is ignored. The threaded variant cannot keep the internal sink alive.

### Check that would have caught it
`ctest --preset tsan -R port_blocking --repeat until-fail:50 --timeout 60` (feature PORT-02-no-hang).

## GTP-05-01-free-text-mistaken-for-header (2026-11-20, owner: claude)

### Symptom
Free text such as `=== Loaded ===` or `??x` during a search opened a reply in `GtpEngine::onLine`; at the next
blank line the search ended with `ErrorEvent{"engine reply carries no id"}` instead of delivering a MessageEvent.

### First fix
None.

### Real source
`parse::parseGtpResponseHeader` accepted any line starting with `=` or `?`. GTP requires marker, optional id
digits, then a space or the end of the line. The engine layer was right to trust the parser.

### Corrected fix
The parser returns nullopt when the char after the optional id is not blank (`src/parse/gtp_parser.cpp`).
`=`, `= text`, `=7`, `=7 K10`, `?`, `? msg` parse as before.

### Check that would have caught it
`gtp_header: a marker glued to other text is not a header` and `gtp5_text` (registered in `tests/CMakeLists.txt`).

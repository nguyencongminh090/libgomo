---
id: PROTO-01-transport-port-stream-transport
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# Transport port and StreamTransport

## Scope
- New library `pygomo_transport` (separate from `pygomo_core`; it may use threads and streams,
  the core may not). Headers under `include/pygomo/transport/`.
- The port: `Transport` (`start(listener)`, `writeLine(line)`, `close()`), `TransportListener`
  (`onLine`, `onClosed`), `TransportError`, `CloseReason` (`Eof`, `Requested`, `LineTooLong`,
  `IoError`). Lines are text without terminator. The port knows nothing about protocols.
- `StreamTransport`: wraps an already-connected `std::istream` / `std::ostream` pair (not owned).
  A reader thread splits input into lines and pushes them to the listener.
  - Strips a trailing `\r`; delivers empty lines (GTP uses a blank line as a terminator); delivers
    a final line that has no newline at EOF.
  - `writeLine` appends `\n` and flushes; writes from several threads never interleave.
  - EOF, `close()` or a read error ends with exactly one `onClosed(reason)`.
  - A line longer than `maxLineLength` (default 1 MiB) ends the stream with `LineTooLong`, so a
    misbehaving engine cannot exhaust memory.
- A `tsan` CMake preset so the concurrency check runs under ThreadSanitizer.

## Don't
- No process spawning, no sockets, no protocol knowledge, no parsing: later todos.
- Do not touch `pygomo_core`; the transport must not depend on it.

## Traps
- A reader blocked in `getline` cannot be interrupted for a generic `std::istream`. `close()` stops
  writing and marks the transport closed; the reader thread ends when the stream reaches EOF or
  fails, so the owner of the underlying stream (pipe, socket) must close it. Document this, and
  make the destructor join only after the stream has ended or been closed by the owner.
- Never call the listener while holding the write mutex (a listener may call `writeLine`).
- The listener runs on the reader thread; document it.

## Done-when
Acceptance lives in `docs/features.json`: one entry per check, `"todo": "PROTO-01"`, description `WHEN <action> THEN <observable result>`, `steps`, `checked_by`, `"passes": false`. Flip `passes` only after running `checked_by`; adding or changing features is the owner's call. `harness-track PROTO-01` lists them.

## Evidence
Clean rebuild of both presets on 2026-11-19 (g++ 15.2, CMake 4.2.3):

- PROTO-01-lines: `ctest --preset default -R transport_lines` passed (2 cases: CRLF/empty/final line, trailing newline adds no extra line).
- PROTO-01-write: `cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan` exit 0, 7/7 passed, 0 "WARNING: ThreadSanitizer" in the log. 8 threads x 100 lines gave exactly 800 distinct complete lines. TSan was confirmed to detect a deliberate race in a probe program in this environment.
- PROTO-01-close: `ctest --preset default -R transport_close` passed: one onClosed(Requested), writeLine returns Closed, a line fed after close never reaches the listener.
- PROTO-01-limit: `ctest --preset default -R transport_limit` passed: 9 chars with limit 8 gives LineTooLong and no later line; exactly 8 chars is accepted.
- PROTO-01-deps: `scripts/check-core-deps.sh` printed OK, exit 0. Negative test: a temporary transport header mentioning a protocol name made it exit 1; removed afterwards.
- Whole suite: 11 transport cases, 848 assertions, 0 failed. 30 repeated runs each of the default and tsan transport tests: 0 failures.

During verification TSan timing exposed a flaw in my own test (second start() on an input that had already hit EOF returned Closed, not AlreadyStarted). The test was made deterministic with an open pipe, and a separate case now pins the Closed behaviour; no check was weakened.

## Links
- features/engine-protocol-port/planning.md (Decision: Concurrency, Process)
- docs/todo/CORE-01-cmake-skeleton-core-types.md

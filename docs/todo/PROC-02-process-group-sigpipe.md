---
id: PROC-02-process-group-sigpipe
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# ProcessTransport: kill the process group, no process-wide SIGPIPE change

## Scope
- Spawn the child in its own process group (`POSIX_SPAWN_SETPGROUP`, group 0) and send SIGTERM/SIGKILL to the group, so an engine started through a wrapper script does not leave grandchildren behind.
- Remove `signal(SIGPIPE, SIG_IGN)`. `writeLine` blocks SIGPIPE on the calling thread for the write, drains a pending one after EPIPE and restores the mask. Update the class comment in `process_transport.hpp`.

## Don't
- Change the `Transport` interface or any other adapter.
- Signal the group after the child exited on its own (its pid could be reused).

## Traps
- A zombie still answers `kill(pid, 0)`; tests must read `/proc/<pid>/stat`.
- SIGPIPE from `write` is thread-directed, so `sigtimedwait` with a zero timeout on the same thread consumes it.
- The child no longer receives terminal Ctrl-C; accepted, the transport ends it itself.

## Done-when
Acceptance in `docs/features.json` (`"todo": "PROC-02"`).

## Evidence
Tests `proc_group`, `proc_sigpipe`, `proc_nohandler` failed first (sigpipe aborted the test process), then pass. `ctest --preset default` and `ctest --preset tsan` all pass (live tests skipped), `scripts/check-core-deps.sh` OK, proc tests stable over 3 runs. Group signalling is only used on the SIGTERM/SIGKILL escalation path, never after the child exited by itself. Grandchildren of a child that exits on its own within the grace period are not touched.

## Links
Source: handoff "Uncertain" point on `ProcessTransport`; PROC-01.

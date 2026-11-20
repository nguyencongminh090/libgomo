---
id: PROC-03-kill-orphans-after-child-exit
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# ProcessTransport: also end the process group when the child exits by itself

## Scope
- In `terminateChild`, observe the child's exit without reaping it (`waitid(P_PID, WEXITED|WNOWAIT)` / `WNOHANG`), then `kill(-pid_, SIGKILL)` while the leader is still a zombie (its pid cannot be reused yet), then reap. Grandchildren of a child that exited on its own no longer survive `close()`/destructor.
- Update the class comment in `process_transport.hpp`.

## Don't
- Change the `Transport` interface, `ProcessOptions` or any adapter.
- Signal the group after the leader was reaped.

## Traps
- A zombie leader still answers `kill(-pid, sig)`; that is what makes this safe.
- The grandchild may hold the child's stdout, so EOF can be late; close() is what ends it.
- Keep the SIGTERM/SIGKILL escalation for a child that does not exit.

## Done-when
Acceptance in `docs/features.json` (`"todo": "PROC-03"`).

## Links
Source: PROC-02 evidence and handoff "Uncertain" point on `ProcessTransport`.

## Evidence
`proc_orphan` failed first (grandchild survived), then passes. `ctest --preset default` and `--preset tsan` 53/53 pass (live skipped), `scripts/check-core-deps.sh` OK, proc tests stable over 3 runs.

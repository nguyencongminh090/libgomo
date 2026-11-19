---
id: PROC-01-process-transport
type: todo
size: M
status: done
owner: claude
created: 2026-11-19
supersedes: []
links: []
planning: engine-protocol-port
---
# ProcessTransport (POSIX)

## Scope
- Library `pygomo_process` (only built `if(UNIX)`), depends on the transport port.
  `ProcessTransport(ProcessOptions)`: program, args, working directory, merge-stderr flag
  (default: stderr to /dev/null), `maxLineLength`, grace periods.
- `posix_spawnp` with stdin/stdout pipes (CLOEXEC); SIGPIPE ignored process-wide once and reset
  to default in the child.
- Reader thread polls the engine pipe and a wake-up pipe, so `close()` interrupts a blocked read
  (this removes the StreamTransport limitation). Same line rules as StreamTransport.
- Shutdown: close stdin, wait, SIGTERM, wait, SIGKILL, always reap. `exitStatus()`.

## Don't
- No protocol knowledge, no Windows, no change to core or the port.

## Done-when
`docs/features.json`, `"todo": "PROC-01"`.

## Evidence
2026-11-19: proc_lines/roundtrip/close/exit/spawnfail/limit passed; suite 26/26 default and tsan; proc_ x20 under tsan clean; check-core-deps OK. Tests use `exec sleep` so SIGTERM reaches the sleeper; a first run with plain `sh -c "sleep 60"` left two orphaned sleeps (the shell forked), a test artefact, not a transport leak: the count did not grow on later runs. ProcessTransport kills only its direct child, not grandchildren.

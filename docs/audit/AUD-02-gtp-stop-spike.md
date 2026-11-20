---
id: AUD-02-gtp-stop-spike
type: audit
size: S
status: active
owner: claude
created: 2026-11-20
supersedes: []
links: []
---
# Spike: can GtpEngine::stop() interrupt a KataGomo genmove? No.
Spike (throwaway read of the KataGomo source, no code kept). A non-bug decision or review. Immutable; supersede it with a new one.

## Context
Handoff listed `GtpEngine::stop()` (and `KataGomoEngine::stop()`) as a no-op because genmove blocks the engine's read loop. Checked against KataGomo `Gom2026` @ `9a0e65c`, `cpp/command/gtp.cpp` and `cpp/command/gomprotocol.cpp`.

## Decision
It cannot be done through the protocol; `stop()` stays a no-op and its comment stays.
- Both front ends read stdin on one thread (`while(getline(cin,line))`, gtp.cpp:1279, gomprotocol.cpp:1466) and run `genmove` and `kata-genmove_analyze` synchronously inside that loop. A `stop` line written during the search is not read until the move is already printed.
- No stdin reader thread and no signal handler (`SIGINT`/`SIGTERM`) exists in these files that could stop a search.
- `stop` only ends pondering and the asynchronous `analyze`/`lz-analyze`/`kata-analyze` commands (`engine->stopAndWait()`), which never produce a played move.
- A search is therefore bounded only by the limits sent before it (`maxTime`, `maxVisits`, `maxmoves`). Killing the process would stop it but also ends the engine; that is `close()`, not `stop()`.

Not pursued: driving the search with `kata-analyze` and stopping it to take the best line. It is a different search mode (the engine does not choose or play the move) and would need its own planning (size L).

## Status
active

## Supersedes
none

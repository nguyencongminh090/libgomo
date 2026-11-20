---
id: core-message-resign-events
type: planning
size: L
status: approved
owner: claude
created: 2026-11-20
supersedes: []
links: [engine-protocol-port]
---
# Message and resign events in the core
Discussion for an L-size feature. Open questions must be empty to approve. After approval it is frozen; split todos (each at most M) carry `planning: core-message-resign-events`.

## Goal
Let applications tell an engine's free text and an engine's resignation apart from "unknown line" and "failure", without reading protocol text.

## Non-goals
- No change to how searches start or stop. No new engine adapters. No banner/out-of-search lines (they are still dropped; separate todo if wanted).
- No game-over handling beyond reporting the resignation.

## Constraints
- Core stays protocol-neutral and I/O-free (`scripts/check-core-deps.sh`); the port rule "exactly one terminal event per search" stays.
- Today: `SearchEvent = variant<Info, BestMove, Swap, Pass, Error, Unparsed>` (`include/pygomo/core/search_event.hpp`).
  Gomocup `MESSAGE` lines that are not an info shape become `UnparsedEvent`; GTP `resign` becomes `ErrorEvent{"engine resigned"}`
  (`src/gtp/gtp_engine.cpp:130`). KataGomo `MESSAGE Visits ... Winrate ...` also lands in `UnparsedEvent`.
- Adding a variant alternative breaks every exhaustive `std::visit` in client code (pre-1.0, only this repo's tests today).

## Options
Messages:
- M1. New non-terminal `MessageEvent{text}`; parsers emit it for `MESSAGE <text>` that is not an info shape. `UnparsedEvent` then means only "line not understood".
- M2. Keep `UnparsedEvent` for messages (no change). Callers filter by prefix.
- M3. Add an optional `message` field to `InfoEvent`. Mixes two meanings; rejected.
Resign:
- R1. New terminal `ResignEvent`; GTP `resign` maps to it. Gomocup has no resign, so only GtpEngine emits it.
- R2. Keep `ErrorEvent{"engine resigned"}`. Callers must match text to tell resign from failure.

## Decision
Approved by owner 2026-11-20: M1 + R1, in two todos (each at most M, build green after each):
1. `MessageEvent` in core + Gomocup parser + adapters and tests updated; `UnparsedEvent` documented as unknown only.
2. `ResignEvent` (terminal) in core + GTP mapping + port contract text and tests updated.
Feature acceptance must name the commands: `ctest --preset default`, `ctest --preset tsan`, `scripts/check-core-deps.sh`.

## Open questions

## Owner answers (2026-11-20)
- `MessageEvent` carries raw text only.
- `ResignEvent` is an ordinary terminal event; no special handling in `search()`.
- Tests that expect `UnparsedEvent` for `MESSAGE` lines (parser fixtures, `gomo_events`, `kata_events`) may change their expectation to `MessageEvent`, nothing else in them.

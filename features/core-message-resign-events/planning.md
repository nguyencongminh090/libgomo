---
id: core-message-resign-events
type: planning
size: L
status: draft
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
Proposed, not approved: M1 + R1, in two todos (each at most M, build green after each):
1. `MessageEvent` in core + Gomocup parser + adapters and tests updated; `UnparsedEvent` documented as unknown only.
2. `ResignEvent` (terminal) in core + GTP mapping + port contract text and tests updated.
Feature acceptance must name the commands: `ctest --preset default`, `ctest --preset tsan`, `scripts/check-core-deps.sh`.

## Open questions
- Does `MessageEvent` carry the raw text only, or also a severity? (Proposal: raw text only.)
- Is `ResignEvent` terminal for the application only, or should `search()` in `blocking_search` expose it separately? (Proposal: it is a terminal event like the others.)
- Existing tests that expect `UnparsedEvent` for `MESSAGE` lines (parser fixtures, `gomo_events`, `kata_events`): owner must approve changing those expectations, since tests may not change otherwise.

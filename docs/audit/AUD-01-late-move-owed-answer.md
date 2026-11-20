---
id: AUD-01-late-move-owed-answer
type: audit
size: M
status: superseded
owner: claude
created: 2026-11-20
supersedes: []
links: []
---
# A timed-out Gomocup search leaves the engine owing an answer
A non-bug decision or review. Immutable; supersede it with a new one.

## Context
GOMO-05 ends a search with ErrorEvent after turnTime + 2 grace. The engine may still print its move later; with a new search already running that move would be taken as the new answer. Gomocup has no request id. Rapfi (`command/gomocup.cpp`, around line 968) drops every command except END/STOP/YXSTOP while it is thinking, so a sync command such as ABOUT gets no reply exactly when it is needed.

## Decision
After a timeout the adapter refuses new searches (`NotReady`) until the late move or swap arrives; that line is dropped. Considered and rejected: ABOUT as a barrier (no reply while thinking); discarding by position check only (a legal late move matches a repeated position). Cost: an engine that never answers stays refused until the caller closes it, which is what the caller should do anyway.

## Status
active

## Supersedes
none

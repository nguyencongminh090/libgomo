---
id: AUD-04-gtp-raw-reply-attribution
type: audit
size: M
status: active
owner: claude
created: 2026-11-22
supersedes: []
links: [GTP-06]
---
# GtpEngine tells raw replies from search replies by their place in the reply order
A non-bug decision or review. Immutable; supersede it with a new one.

## Context
GTP-06: `GtpEngine::sendRaw` writes the line as is, so its reply has no id. `onReply` treated any id-less
reply during a search as a failure and ended the search with ErrorEvent. The guide recommends `sendRaw` for
`kata-set-param` keys, exactly this case. An id-less `?` is also how an engine that does not echo ids
reports a failing setup command, so id-less replies cannot simply be ignored.

## Decision
GTP answers commands one at a time and in order. `GtpEngine` keeps a queue of unanswered commands in write
order, each marked search or raw; every complete reply takes the front entry. A reply whose entry is raw (with
or without an id) is delivered as `MessageEvent{"=[id] body"}` or `MessageEvent{"?[id] body"}` during a
search, dropped otherwise, and never ends the search. A reply whose entry is a search command, or that comes
with an empty queue, goes through the old checks, so an id-less `?` for a setup line still ends the search
with ErrorEvent. Empty and comment lines are not queued (GTP 2 section 2.4 gives them no reply). A
`writeMutex_` (taken before `mutex_`) is held over each queue push and its write in `startSearch` and
`sendRaw`, so the queue matches the wire order across threads; a failed write takes its unwritten entries
back off the end. `startSearch` no longer resets reply framing, so a raw reply whose header was read before
the search starts still ends at its blank line.

Considered and rejected: giving raw commands an id (breaks "sends a line as is", doubles a caller's own id,
and the reply would be dropped as stale); ignoring every id-less reply during a search (hides real setup
failures of engines that drop ids); a count of pending raw commands (first version, found in review: a raw
line written after the search's lines made an id-less setup failure count as the raw reply).

Cost: the attribution relies on one reply per non-empty, non-comment line. An engine that skips a reply, or a
line its preprocessing empties (control characters only), shifts every later attribution until close.

## Status
active

## Supersedes
none

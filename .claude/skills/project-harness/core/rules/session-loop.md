# Session loop (advisory)

Start of a session, before new work: read `docs/handoff.md`, `git log --oneline -10`, and the active todo with its features (`harness-track.mjs <CODE>`); run the Smoke command from `CLAUDE.md`. If the smoke test already fails, fix or report that first.

During: one feature per session (a todo may have several features; finish one, verify, then the next). Commit after each verified feature with a message that says what changed and why; do not leave a half-done state uncommitted at the end.

Never remove or edit a test, or weaken a feature's `checked_by`, to make something pass. Adding tests is fine. If a check is wrong, tell the owner.

Verify user-visible behaviour the way a user would (real browser or the real command), not only with unit tests or curl; unit tests alone miss integration bugs.

End: overwrite `docs/handoff.md`. For a second opinion on a finished todo, use the `reviewer` agent (`.claude/agents/reviewer.md`).

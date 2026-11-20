---
id: HARNESS-04-passes-only-semantic-check
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# Semantic check for the passes-only edit

## Scope
The `passes`-only check normalises Edit/MultiEdit text with a regex. Apply the edit and compare the JSON semantically; unknown tool names fall back to `ask`.

## Don't
- Touch anything outside the vendored harness scripts (`.claude/skills/project-harness/scripts/`) and their tests; no C++ changes.
- Add or change features without the owner.

## Done-when
Acceptance in `docs/features.json` (`"todo": "HARNESS-04"`), proposed and owner-approved before code.

## Evidence
Tests `HARNESS-04-semantic`, `HARNESS-04-unknown` in `tests/minor-fixes.test.mjs`; suite 143/143. Edit/MultiEdit are applied to the current text and compared as JSON; non-boolean `passes` counts as a change; unknown tools ask. An old_string absent from the file (the tool rejects it) falls back to the old/new passes-normalised comparison, which the existing v03 tests require.

## Links
Source: security review findings, formerly `docs/notes/2026-11-19-harness-gate-review-findings.md` (finding 04, deleted on promotion).

---
id: HARNESS-04-passes-only-semantic-check
type: todo
size: M
status: active
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
(fill in before marking done)

## Links
Source: security review findings, formerly `docs/notes/2026-11-19-harness-gate-review-findings.md` (finding 04, deleted on promotion).

---
id: HARNESS-03-canonical-features-path
type: todo
size: M
status: active
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# Canonical path for the features.json match

## Scope
The `features.json` match uses `path.resolve`. Use `fs.realpathSync` and case-fold on case-insensitive platforms.

## Don't
- Touch anything outside the vendored harness scripts (`.claude/skills/project-harness/scripts/`) and their tests; no C++ changes.
- Add or change features without the owner.

## Done-when
Acceptance in `docs/features.json` (`"todo": "HARNESS-03"`), proposed and owner-approved before code.

## Evidence
(fill in before marking done)

## Links
Source: security review findings, formerly `docs/notes/2026-11-19-harness-gate-review-findings.md` (finding 03, deleted on promotion).

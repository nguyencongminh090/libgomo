---
id: HARNESS-01-profile-name-validation
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# Validate profile names before import()

## Scope
Profile names from `harness.json` reach an `import()` path unchecked. Validate with `/^[a-z0-9][a-z0-9-]*$/` and keep the resolved directory inside the allowed roots.

## Don't
- Touch anything outside the vendored harness scripts (`.claude/skills/project-harness/scripts/`) and their tests; no C++ changes.
- Add or change features without the owner.

## Done-when
Acceptance in `docs/features.json` (`"todo": "HARNESS-01"`), proposed and owner-approved before code.

## Evidence
(fill in before marking done)

## Links
Source: security review findings, formerly `docs/notes/2026-11-19-harness-gate-review-findings.md` (finding 01, deleted on promotion).

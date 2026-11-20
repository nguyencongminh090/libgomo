---
id: HARNESS-01-profile-name-validation
type: todo
size: M
status: done
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
Tests `HARNESS-01-name`, `HARNESS-01-root` in `tests/minor-fixes.test.mjs`; full harness suite 138/138; `harness-check` 0 errors. Names validated in `loadProfileChecks`/`findingsFor` (new finding `PROFILE-NAME`); symlink escape skipped via realpath. Installed copy `scripts/harness` synced.

## Links
Source: security review findings, formerly `docs/notes/2026-11-19-harness-gate-review-findings.md` (finding 01, deleted on promotion).

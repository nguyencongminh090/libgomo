---
id: HARNESS-02-gate-control-config-ask
type: todo
size: M
status: done
owner: claude
created: 2026-11-20
supersedes: []
links: []
planning: 
---
# Edit gate asks for control config

## Scope
The edit gate silently allows `harness.json`, `.claude/` and `docs/`, so overrides, hooks, rules and planning status are editable without the owner. Make those return `ask`, like `features.json`.

## Don't
- Touch anything outside the vendored harness scripts (`.claude/skills/project-harness/scripts/`) and their tests; no C++ changes.
- Add or change features without the owner.

## Done-when
Acceptance in `docs/features.json` (`"todo": "HARNESS-02"`), proposed and owner-approved before code.

## Evidence
Tests `HARNESS-02-ask`, `HARNESS-02-exempt` in `tests/minor-fixes.test.mjs`; suite 140/140; `harness-check` 0 errors. Gate asks for `harness.json`, `.claude/**`, `docs/**` and `features/*/planning.md`; exempt: `docs/handoff.md`, `docs/fix-log.md`, `docs/todo/**`, `docs/notes/**`. Planning files added to the ask set because planning status is the L gate.

## Links
Source: security review findings, formerly `docs/notes/2026-11-19-harness-gate-review-findings.md` (finding 02, deleted on promotion).

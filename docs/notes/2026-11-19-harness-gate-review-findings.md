---
id: 2026-11-19-harness-gate-review-findings
type: note
size: S
status: active
owner: claude
created: 2026-11-19
supersedes: []
links: []
---
# Harness gate: security review findings (to become a todo)
Temporary scratch for an S-size task. Delete it, or promote it to a todo or fix-log. Never keep decisions here.

## What
An automated security review of the vendored project-harness scripts
(`.claude/skills/project-harness/scripts/`, `harness-check.mjs`, `lib/checks/features.mjs`)
reported four medium findings. Nothing was changed; the owner agreed to track them separately,
after PROTO-01. Promote to a todo once the owner picks a domain (current domains are only
CORE, PROTO, PARSE, none fits harness tooling).

1. Profile names go unchecked into an `import()` path built from `harness.json`; validate with
   `/^[a-z0-9][a-z0-9-]*$/` and keep the resolved directory inside the allowed roots.
2. The edit gate allows `harness.json`, `.claude/` and `docs/` without asking, so the control
   config (overrides, hooks, rules, planning status) is editable without the owner. Make those
   return `ask`, like `features.json`.
3. The `features.json` match uses `path.resolve`, not canonical paths; use `fs.realpathSync` and
   case-fold on case-insensitive platforms.
4. The `passes`-only check normalises Edit/MultiEdit text with a regex instead of applying the
   edit and comparing the JSON semantically; unknown tool names should fall back to `ask`.

## Outcome
(fill in when promoted or closed)

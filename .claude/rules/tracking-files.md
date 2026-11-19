---
paths:
  - "TODO.md"
  - "docs/**"
  - "features/**"
---
# Tracking files

- A todo's frontmatter `status` (`backlog|active|done|archived`) is the truth; `TODO.md` is a derived index. After changing a status run `harness-check --fix`.
- Codes are `<DOMAIN>-<NN>-<slug>` for todo, design, audit, adr and fix-log; slug is English kebab-case. Never reuse a code, archived ones stay reserved.
- Acceptance lives in `docs/features.json`. Done means every feature of the todo has `"passes": true`. Flip `passes` only after running its `checked_by`; never add, edit or remove features without the owner (the PreToolUse hook asks). Legacy Done-when checkboxes, if present, must also be ticked.
- Audits and ADRs are immutable: write a new one that `supersedes` the old.
- Reports, screenshots and data go to `docs/reports/`, never the docs root. Finished work goes to `docs/archive/` via `harness-archive`.
- One entity, one folder. No `audits/`, `instruction/` or a second todo folder.

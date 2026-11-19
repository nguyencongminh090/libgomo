# Docs map

This folder follows the project-harness standard. One entity, one folder.

| Path | What lives here |
|---|---|
| `../TODO.md` | Index lines only (backlog / active / done). Derived from todo frontmatter; `harness-check --fix` regenerates it. |
| `todo/` | One file per doable item, `<DOMAIN>-<NN>-<slug>.md`. Frontmatter `status` is the truth. |
| `../features/<slug>/planning.md` | Planning for an L-size feature. Must be approved before its todos start. |
| `design/` | Settled designs of parts. |
| `adr/` | Hard-to-reverse architecture decisions. Immutable; supersede, never edit. |
| `audit/` | Non-bug decisions and reviews. Immutable; supersede, never edit. |
| `notes/` | Expiring S-size notes. Delete or promote. |
| `templates/` | The document templates. Copy one, fill the frontmatter. |
| `fix-log.md` | Append-only list of wrong-layer bug root causes (details in `fix-log/` if used). |
| `handoff.md` | Where the work stands between agent sessions. Overwritten each session. |
| `archive/` | Finished todos, closed audits, old reports (`archive/<yyyy>/`). Codes stay reserved. |
| `reports/` | Generated reports, screenshots, data. Never in the docs root. |

## Commands (run from the project root)

- `node scripts/harness/harness-check.mjs [--fix] [--json]` - lint the docs; exits 1 on any error.
- `node scripts/harness/harness-track.mjs <CODE>` - print one item.
- `node scripts/harness/harness-archive.mjs [--apply]` - move old done items to the archive (dry run by default).
- `node scripts/harness/harness-diff.mjs --skill <path-to-project-harness>` - report drift from the standard.

## Status flow

`backlog -> active -> done -> archived` (todo frontmatter is the status truth). Acceptance lives in `docs/features.json` (one entry per check, `passes` flag); a todo is done only when all its features pass.

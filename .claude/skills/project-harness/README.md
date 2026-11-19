# project-harness (authored in SKILLS_TREE, 2026-11-04)

Authored in the tree, not copied from a project, so there are no hardcoded project references. Spec: `docs/superpowers/specs/2026-11-04-project-harness-design.md`; plan: `docs/superpowers/plans/2026-11-04-project-harness.md` (in SKILLS_TREE).

## Copy and adapt
- Copy the whole folder: `tools/find --copy <dest> --pick project-harness`. The `tests/` folder may be dropped in a project.
- Requires Node >= 18, no packages. Never use python for these scripts.
- `harness-init` copies the scripts into the target project at `scripts/harness/`; the project then runs its own copy.
- `harness.json` is JSON on purpose (Node reads it without a dependency); the design spec's first draft said YAML.
- Tests: `node --test tests/*.test.mjs` (Node 22 needs the glob, not a folder).

## Known baseline (2026-11-04, harness-check 0.1.0)
Read-only copies of existing projects' docs (they predate the standard, so findings are expected; the point is no crash and a fast scan). Neither project has a harness.json, so each gets one CONFIG-MISSING warn.

| Project | Files scanned | Time | Findings | Errors | Breakdown |
|---|---|---|---|---|---|
| Rapfi V2 (docs, TODO.md, CLAUDE.md) | 30 | 0.08 s | 18 | 16 | 12 FM-MISSING, 3 LINK-BROKEN, 1 CLAUDE-LONG (error), 1 LEGACY-DIR, 1 CONFIG-MISSING |
| gomoku-vn (docs, TODO.md, CLAUDE.md) | 1172 | 0.16 s | 1736 | 1728 | 1341 LINK-BROKEN, 386 FM-MISSING, 5 DOCS-ROOT-CLUTTER, 2 LEGACY-DIR, 1 CLAUDE-LONG (error), 1 CONFIG-MISSING |

## Changes since 0.3.0 (review against Anthropic guidance, 2026-11-05)
Review and status of every finding: `docs/superpowers/specs/2026-11-05-project-harness-anthropic-review.md` (in SKILLS_TREE).
- 0.3.1: Stop hook ignores errors present at session start; a crashing hook warns instead of failing silently; description and `${CLAUDE_SKILL_DIR}` paths; `webapp-client` rule with `paths:`.
- 0.3.2: exec-form hooks with timeout, NotebookEdit, `permissions.deny`, `harness-init --hooks` (idempotent merge), read-only `reviewer` agent, `session-loop` rule, Smoke line in the CLAUDE.md template, `.gitignore` entries.
- 0.3.3: UserPromptSubmit reminder (`harness-check --prompt`) so small models record mid-conversation ideas.

## Behaviour evals (cost tokens, not part of `node --test`)
`node tests/evals/run-evals.mjs [--models haiku,sonnet,opus] [--scenarios L,M,REQ] [--reps N] [--control]` runs `claude -p` on a throwaway project (L request, M todo, mid-conversation idea) and grades the files. Results are summarised in the review document. Temp projects stay in the OS temp dir; delete them by hand. Not yet run on a real project.

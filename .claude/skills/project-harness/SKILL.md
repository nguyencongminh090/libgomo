---
name: project-harness
description: Sets up the workspace and agent process of a new large project (a workflow standard for AI coding agents): task sizes S/M/L (small, medium, large) with an approval gate before code, new requirements recorded instead of built, features.json acceptance checklist with a passes flag, todo/planning/fix-log templates, TODO.md index, docs layout, archive rules, Node scripts (init, check, track, diff, archive) and composable project-type profiles. Use when starting a long-running agent project or auditing a project's docs against the standard; not for one-off scripts or short-lived projects (use lite-harness).
---

# project-harness

One standard for the workspace of a future project so that coding agents follow a clear process: size the work, produce the documents that size needs, pass the gate before code, verify, record, archive. Core + small profiles per project type + a zero-dependency Node kit that checks the structure.

Full process rules (why and what): `rules/methodology/items/project-harness-process.md` in SKILLS_TREE. Do not duplicate them here.

## When to use
- Starting a new project that will run for a long time with agents, and you want docs/todo/planning structure from day one.
- A project's docs drifted (duplicate folders, no archive, unchecked rules) and you want a target standard to compare against (`harness-diff`, `harness-check`).

Do not use for a one-off script or a throwaway spike: the ceremony costs more than the work. For short-lived projects (assignment, demo, fixed-requirements waterfall) use `lite-harness`.

## Requirements
Node >= 18. No npm packages. Copy this folder into the target workspace (or run from SKILLS_TREE) and call scripts by path.

## Start a project
1. Pick profiles that fit (read each `profiles/<name>/profile.json` `when_to_use`): `webapp`, `software-app`, `engine-perf`, `module-architecture`, `library`, `ai-implementation`. Zero profiles is fine.
2. `node "${CLAUDE_SKILL_DIR}/scripts/harness-init.mjs" --root <project> --profile webapp --domains WEB,API --hooks`
   Installs templates, rules, the read-only `reviewer` agent (`.claude/agents/`), `CLAUDE.md`, `.gitignore` entries (`CLAUDE.local.md`), `TODO.md`, the docs skeleton, `harness.json` and a copy of the scripts at `<project>/scripts/harness/`. Never overwrites; it lists `skipped:` files. Conflicting profiles make it refuse.
3. Edit the project's `CLAUDE.md`: fill the `Commands` slot first (build, test, single test, lint), keep it under 50 lines (checker caps at 60), and write only what an agent cannot find by reading the repo. Other agents too: put shared rules in `AGENTS.md` and make the first line of `CLAUDE.md` `@AGENTS.md` (no symlink; Claude Code ignores AGENTS.md when CLAUDE.md exists).
   `--hooks` merges `core/hooks/settings.sample.json` into `.claude/settings.json` (keeps existing hooks, adds each harness hook once, adds `permissions.deny` for `.env*` and `secrets/`; refuses to touch invalid JSON). Without the flag nothing is installed; merge by hand if you prefer. It wires SessionStart (`--session-start`: handoff + active items + check counts into context), PreToolUse (`--gate`: blocks code edits behind an unapproved L planning) and Stop (`--stop`: exit 2 blocks finishing on errors that are new since session start; the second stop is allowed; a crashing hook warns and lets the action go, add `"onFailure": "block"` to the gate hook to make it strict, Claude Code 2.1.295+).
4. Daily loop, from the project root:
   - Now and then run `/doctor prompt-audit` in Claude Code: it flags stale or conflicting lines in CLAUDE.md and rules.
   - `node scripts/harness/harness-check.mjs` (exit 1 on any error; `--fix` regenerates `TODO.md`, `--json` for machines)
   - `node scripts/harness/harness-track.mjs <CODE>` prints one item (and its features with PASS/FAIL)
   - `node scripts/harness/harness-archive.mjs [--apply]` moves old done items (dry run by default)
   - `node scripts/harness/harness-diff.mjs --skill "${CLAUDE_SKILL_DIR}"` (only the init call and this one use the skill folder; the rest run from the copy in the project) reports drift from the standard (report only)

## Size model (decide first; when unsure pick the larger)
| Size | Documents | Gate before code |
|---|---|---|
| S | expiring `note` | none |
| M | `todo` + its features (`docs/features.json`, `passes` flag) | read the todo |
| L | `planning` (the L unit, never a todo) -> approved (open questions empty) -> todos of at most M -> `audit`/`adr` | planning approved |

Acceptance lives in `docs/features.json` (Anthropic pattern: JSON is rewritten less than Markdown). Each feature: `id`, `todo` code, `description` (WHEN ... THEN ...), `steps`, `checked_by`, `passes`. Agents flip only `passes`, after running `checked_by`; any other edit there makes the PreToolUse hook ask the owner. A todo is done only when all its features pass.

Bug = fixed chain reproduce -> root cause -> kept regression check -> `fix-log` entry. Spike = throwaway, labelled. New requirement mid-conversation: record it, do not build it until told.

## Unknown project type
No profile fits? Interview the owner (main risks, what is hard to reverse, what "done" means, how it is verified), save the answers as JSON, run `node "${CLAUDE_SKILL_DIR}/scripts/harness-init.mjs" --new-profile <name> --answers <file>`. It writes a draft profile (`"status": "draft"`) under `profiles/`; the owner approves it before use. See `profiles/PROFILE-SPEC.md`.

## Adding to the standard
Profiles are data: add `profiles/<name>/profile.json` plus `rules/*.md`; never edit the core for one project type. Run `node --test tests/*.test.mjs` after changes.

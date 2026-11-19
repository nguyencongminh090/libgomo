# <Project name>

<One line: what this project is.> Keep this file under 50 lines. Write only what an agent would not find by reading the repo: exact commands, non-standard conventions, traps, decisions. Process detail lives in `.claude/rules/` (loaded automatically) and `docs/README.md`.
<!-- Other agents too? Put shared rules in AGENTS.md and make the first line of this file `@AGENTS.md`; keep Claude-only lines below it. Do not symlink CLAUDE.md to AGENTS.md. Personal notes go in CLAUDE.local.md (git-ignored by harness-init). Run /doctor prompt-audit now and then to catch stale or conflicting lines. -->

## Commands
- Build: `<command>`
- Start / smoke (run before new work; must pass): `<command>`
- Test all: `<command>`
- Single test: `<command> <file>`
- Lint / typecheck: `<command>`
- Harness check (must exit 0 before finishing): `node scripts/harness/harness-check.mjs`

## Hard rules (machine-checked by `scripts/harness/harness-check.mjs`)
- Every task has a code `<DOMAIN>-<NN>-<slug>`; `TODO.md` holds index lines only, detail is in `docs/todo/`.
- L work is a `planning` (`features/<slug>/planning.md`), never a todo. Its todos (each at most M, `planning: <slug>`) start only after it is approved: no open questions, no `[NEEDS CLARIFICATION: ...]`.
- Acceptance lives in `docs/features.json`. A done M todo has at least one feature, and all its features have `"passes": true`.
- `docs/` root holds no reports, screenshots or data; they go in `docs/reports/`.

## Task size
Decide S / M / L before touching anything; when unsure pick the larger; if a task grows, stop and step up. Table and gates: `.claude/rules/task-size.md`.

## Advisory rules (not machine-checked)
- New requirement raised mid-conversation: record it, do not build it until told to (do this now / implement / fix it).
- Bug: reproduce -> root cause -> kept regression check -> fix-log entry. Scope is only what was reported.
- Spike (can this be done?): throwaway and labelled; keeping it is a new request that goes through sizing.
- Do not store decisions in notes.
- In `docs/features.json` flip only `passes`, only after running `checked_by`; adding or changing features is the owner's call (with hooks, Edit/Write there asks the owner; Bash edits are not covered).
- Before marking a todo done, put the command you ran and its output under `## Evidence`.

## Pointers
- Where things live: `docs/README.md`. Show one item: `node scripts/harness/harness-track.mjs <CODE>`.
- Session state: `docs/handoff.md` (overwrite at session end). Wrong-layer bugs: `docs/fix-log.md`.

## Project-specific
<Non-standard conventions and traps only. Delete this section if empty.>

# libgomo agent guide

## Project
libgomo is a C++ framework/protocol layer connecting applications to Gomoku engines.
Priorities: low coupling, multiple protocols, open interfaces for third-party protocols, and clean code.
Engine-output text parsing must robustly support multiple formats, from abstract to detailed representations.
No source code or build system exists yet; do not assume compiler, dependency, or test commands.

## Read first
- Read `CLAUDE.md`, `harness.json`, `docs/README.md`, and `.claude/skills/project-harness/SKILL.md`.
- Follow `.claude/rules/*.md` for sizing, tracking, sessions, architecture, bug fixes, and spikes.
- Start with `docs/handoff.md`, recent git history when available, `TODO.md`, and the active todo.
- Consult the `docs/knowledge/` index (filenames) and read relevant notes as needed.
- Before inventing workflows, design decisions, or review checklists, use the SKILLS_TREE search described in `CLAUDE.md`.

## Size and scope
Size by risk and ambiguity before work; choose the larger size when unsure. Stop and resize if scope grows.

- S: reversible, one layer, no design decision; use an expiring note in `docs/notes/`.
- M: use a todo in `docs/todo/` and its acceptance features; read the todo before code. Record decisions in an audit.
- L: interfaces, protocols/formats, cross-layer changes, toolchain/process changes, or hard-to-reverse decisions.
  Write `features/<slug>/planning.md`; obtain owner approval with no open questions before code.
  Split approved work into todos of at most M with `planning: <slug>`; never make an L-size todo.
- Record newly raised requirements instead of building them until explicitly instructed: L ideas become planning; others become todos.
- For module boundaries or cross-layer dependencies, write a design before code, record an ADR, and specify forbidden dependencies in acceptance checks.

## Execute and verify
- Work on one acceptance feature at a time; run the smoke check once defined and report existing failures.
- Acceptance lives in `docs/features.json`: agents change only `passes`, after running the feature's `checked_by`.
  Adding, editing, or removing other feature fields requires the owner; never weaken checks or tests to pass.
- A todo is done only when all its features pass; record verification evidence in the todo.
- Run `node scripts/harness/harness-track.mjs <CODE>` to inspect a todo and its acceptance features.
- Run `node scripts/harness/harness-check.mjs` to validate the harness.
- Todo frontmatter `status` is authoritative; `TODO.md` is a derived index. Domains: `CORE`, `PROTO`, `PARSE`; never reuse codes.
- Use `docs/templates/`; keep ADRs/audits immutable by superseding them, and fix-log entries append-only.
- Bugs: reproduce, identify the root cause, retain a regression check, and record a fix-log entry. Log unrelated issues separately.
- Label spikes as throwaway; record findings, and resize any request to retain their code.
- Verify real user-facing behavior and commit each verified feature when git is available.
- End normal work sessions by updating `docs/handoff.md`; put reports in `docs/reports/` and finished records in `docs/archive/`.

## Commands
- Build: `cmake --preset default && cmake --build --preset default` (Debug, warnings are errors)
- Test: `ctest --preset default`
- Single test: `./build/default/tests/gomo_core_tests -tc="<test case name>"`
- Core dependency check: `scripts/check-core-deps.sh`
- Smoke: `ctest --preset default -R smoke`
- ThreadSanitizer: `cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan`
- Lint: none yet (warnings as errors act as the check).

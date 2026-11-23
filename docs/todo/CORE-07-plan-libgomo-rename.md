---
id: CORE-07-plan-libgomo-rename
type: todo
size: M
status: done
owner: codex
created: 2026-11-23
supersedes: []
links: [rename-libgomo]
---
# Plan the PyGomo rename to libgomo

## Scope

Prepare the [rename plan](../../features/rename-libgomo/planning.md) for owner approval.
Inventory the integration names that need to change, settle compatibility for existing
consumers, and define the implementation slices and how to verify them.

## Don't

Keep this todo scoped to the migration plan. Implement the rename in the todos that follow
approval.

## Done-when

The owner approves the plan with the public names, compatibility policy, and verification
steps recorded. The plan is marked `approved` with its open questions resolved.

Acceptance entry `CORE-07-plan` is in `docs/features.json`.

## Evidence

- The plan inventories public C++, CMake, and test names, fourteen acceptance commands that
  depend on them, and the source and generated documentation to update.
- `ctest --preset default -R smoke` passed. `ctest --preset default` passed 112 of 119 registered
  tests. The seven live Rapfi entries skipped. `scripts/check-core-deps.sh` passed.
- A fresh standalone CMake project used `add_subdirectory` with the checkout path, found all
  eight `pygomo::` aliases on Unix, and linked only `pygomo::gomocup`. Its executable created a
  `Position`, instantiated `StreamTransport` and `GomocupEngine`, and checked default-rule support.
- The owner approved direct migration and the `CORE-07-plan` acceptance entry on 2026-11-23.
  `AUD-09` records the decision. `CORE-08`, `CORE-09`, and `CORE-10` track implementation.
- The `CORE-07-plan` acceptance command confirmed the approved plan and passed the harness
  check with zero findings. All fourteen affected check IDs were cross-checked against
  `docs/features.json`.

---
id: FMT-02-apply-format-check
type: todo
size: M
status: done
owner: claude
created: 2026-11-23
supersedes: []
links: []
planning: clang-format
---
# Apply clang-format once and add the format check script

## Scope
- One `style: clang-format` commit applying the config to `include/ src/ tests/`; if `tests/` is noisy (see FMT-01 audit) split it per directory.
- Add `scripts/check-format.sh` (`clang-format --dry-run -Werror`, run by hand) and list it in `CLAUDE.md` Commands.
- Update `.claude/rules/coding-style.md`: drop the 20-character outlier exception (planning option O1); keep the `/align` rule until FMT-04.

## Don't
- No logic, rename or reorder change. `third_party/` is never touched. Keep the reformat commit apart from the script and rule edits.

## Done-when
Features `FMT-02-*` in `docs/features.json` (`harness-track FMT-02`).

## Evidence
Format commit applied (includes kept in order via `SortIncludes: Never`), build + ctest 82/82 pass, `scripts/check-format.sh` exits 0 and 123 after a misaligned edit. `diff -w` shows line re-wrapping (accepted per AUD-06).

## Links
`features/clang-format/planning.md`

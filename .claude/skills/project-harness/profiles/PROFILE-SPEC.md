# Profile spec

A profile is a small overlay for one project type. The core never changes to support a new type: add a folder here instead. Profiles are declarative data; check scripts are optional.

## Layout

```
profiles/<name>/
  profile.json        required
  rules/*.md          optional, path-scoped rules installed to .claude/rules/
  templates/*.md      optional, extra document templates installed to docs/templates/
  checks/*.mjs        optional, extra checks (each exports run(ctx) -> Finding[])
```

## Path-scoped rules

A rule file may start with YAML frontmatter `paths:` (a list of globs). Claude Code then loads it only when it works on matching files; `paths` is the only frontmatter field it reads. Use it only for rules tied to a folder (webapp's `webapp-client.md`). Rules about decisions or sizes (library, engine-perf, ...) have no folder, so they stay always-loaded and short.

## profile.json

| Key | Type | Meaning |
|---|---|---|
| `name` | string, required | Folder name |
| `description` | string, required | One line |
| `when_to_use` | string | When the interview should pick it |
| `conflicts` | string[] | Profiles that cannot be combined; `harness-init` refuses |
| `size_signals` | `{pattern, size: "M"\|"L"}[]` | Changes matching `pattern` get at least this size |
| `definition_of_done` | string[] | Extra acceptance checks every todo of this project should consider; add them as features in `docs/features.json` |
| `checks` | string[] | Names of `checks/*.mjs` modules to run in `harness-check` |
| `weakens` | string[] | Core rule ids this profile relaxes (needs an override) |
| `status` | `"draft"` or `"approved"` | Drafts come from the unknown-type interview |
| `confidence` | string | `"low"` when no real project informed it |

## Merge rules

1. Profiles apply in the order listed in the project's `harness.json` `profiles`.
2. Size signals with the same `pattern`: the stricter size wins (`L` over `M`), whatever the order.
3. Rules, checks and definition-of-done lines are added; duplicates are dropped.
4. A profile may add rules but may not weaken a core hard rule. Core rule ids: `claudeMaxLines`, `lNeedsPlanning`, `doneNeedsChecks`. If a profile lists one in `weakens`, the project must name it in `harness.json` `overrides` (for example `"claudeMaxLines": 90`), otherwise the merge throws `profile <x> weakens core rule <id> without override`.
5. If either profile lists the other in `conflicts`, the merge throws `profile conflict: a vs b`.

## Worked example

`profiles/library/profile.json`:

```json
{
  "name": "library",
  "description": "A package other code depends on",
  "when_to_use": "Public API, semver, consumers outside the repo",
  "conflicts": [],
  "size_signals": [{ "pattern": "public API change", "size": "L" }],
  "definition_of_done": ["changelog entry written"],
  "confidence": "low"
}
```

Project `harness.json`:

```json
{ "version": "0.1.0", "profiles": ["library", "ai-implementation"], "domains": ["LIB"], "overrides": {} }
```

Merged result: a public API change is size L in the whole project; a prompt or model change (from `ai-implementation`) is at least M; both profiles' acceptance checks (features) and rules apply.

## Unknown project type

`harness-init` does not guess. The agent interviews the owner (main risks, what is hard to reverse, what "done" means, how it is verified), writes the answers to a JSON file, and runs `harness-init --new-profile <name> --answers <file>`. That saves a draft (`"status": "draft"`) which the owner approves before it counts.

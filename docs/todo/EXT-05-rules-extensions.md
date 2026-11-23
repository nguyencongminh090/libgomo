---
id: EXT-05-rules-extensions
type: todo
size: M
status: backlog
owner: claude
created: 2026-11-23
supersedes: []
links: [extensible-engine-commands]
planning: extensible-engine-commands
---
# Core: Rules::extensions

## Scope
Add `std::map<std::string, Value> extensions` as the last member of `Rules`; `operator==` compares it; `validate` accepts it. Adapters without an encoder return `supports() == false` and `Unsupported` when it is non-empty.

## Don't
Do not change `BoardRule` or `OpeningRule`. No Caro knowledge in core.

## Traps
Rules is an aggregate: designated initialisers in existing code must still compile.

## Done-when
Features `EXT-05-*` in `docs/features.json` (`harness-track EXT-05`).

## Evidence
(fill in before marking done)

## Links
`features/extensible-engine-commands/planning.md`

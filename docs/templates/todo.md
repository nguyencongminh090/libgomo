---
id: <DOMAIN>-<NN>-<slug>
type: todo
size: M
status: backlog
owner: <person-or-model>
created: YYYY-MM-DD
supersedes: []
links: []
planning: 
---
# Todo title

## Scope
What to do, and only that.

## Don't
What not to touch.

## Traps
Known pitfalls (optional but valuable).

## Done-when
Acceptance lives in `docs/features.json`: one entry per check, `"todo": "<DOMAIN>-<NN>"`, description `WHEN <action> THEN <observable result>`, `steps`, `checked_by` (command or manual step), `"passes": false`. Flip `passes` only after running `checked_by`; adding or changing features is the owner's call. `harness-track <CODE>` lists them.

## Evidence
Command(s) run and the relevant output, filled in before marking done.

## Links
Related files, designs, audits.

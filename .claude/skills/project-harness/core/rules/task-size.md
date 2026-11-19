# Task size

Decide the size before touching anything. Size comes from risk and ambiguity, not file count. Signals: is a design decision involved, is it reversible, does it touch a protocol, format, data or public interface, does it cross layers. When unsure, pick the larger size. If a task grows mid-way, stop, say so, and step up one size.

| Size | Signal | Documents | Gate before code |
|---|---|---|---|
| S | few files, one layer, reversible, no decision | expiring `note` | none |
| M | real work, nothing hard to reverse | `todo` + its features in `docs/features.json` | read the todo; `audit` only if a decision was made |
| L | cross-layer, interface, format, toolchain or process, hard to reverse | `planning` (the L unit, never a todo) -> approved -> todos (each at most M, `planning: <slug>`) -> `audit` or `adr` | planning approved, open questions empty |

Machine-checked: a todo cannot be size L (`TODO-SIZE-L`); a todo that names a planning cannot be active or done until that planning is approved with no open questions (`GATE-L-NO-PLANNING`, `PLANNING-OPEN`). With hooks, both block code edits.

Advisory: a new requirement raised mid-conversation, record it, do not build it until told to ("do this now", "implement", "fix it"). An L idea becomes a planning, anything else a todo. Do not store decisions in notes.

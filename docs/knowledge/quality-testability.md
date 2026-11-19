---
name: quality-testability
description: Testability as an architecture quality - controllability and observability, the two tactic groups from SAiP (add test access, limit complexity), and how architectural style supports it
domain: software-architecture
tags: quality-attributes,testability,controllability,observability,testing
apply_when: "tests are slow, flaky or impossible without real infrastructure, or you are choosing structure (ports, seams, interfaces) with tests in mind"
sources: "ISO/IEC 25010:2023 (testability under maintainability, per secondary sources); Bass, Clements, Kazman - Software Architecture in Practice, 3rd ed., ch. 4 (testability stimulus) and ch. 10 (tactics for testability: listed in TOC, chapter itself not read); Binder - Design for testability in object-oriented systems, CACM 37(9), 1994 (existence confirmed, not read); Cockburn - Hexagonal Architecture (2005)"
last_reviewed: 2026-09-21
confidence: medium
---

# Testability

## Definition

ISO 25010:2023 lists testability as a sub-characteristic of maintainability: how effectively
test criteria can be established for a system, product or component, and tests performed to
determine whether they are met. In the SAiP scenario form, the stimulus for testability is the
completion of a development phase; the response and its measure are yours to specify (for
example, the effort needed to test a change, or how much can be tested without real infrastructure).

Two properties dominate: **controllability** (can you put the component in a chosen state
and give it inputs?) and **observability** (can you see its outputs and state?). Binder's 1994
paper on design for testability in object-oriented systems is a primary reference for this
framing (existence confirmed; not read in full).

## Tactic groups (SAiP, ch. 10; summarized from secondary sources)

1. **Add controllability and observability**: specialized interfaces (get, set, reset,
   report), record and playback of data crossing an interface, sandboxing or virtualized
   resources, executable assertions.
2. **Limit complexity**: limit structural complexity (separation of concerns, low coupling)
   so components can be tested in isolation; limit non-determinism, for example by
   abstracting the system clock from wall-clock time so time-dependent code can be tested at
   boundaries. (The SAiP acknowledgments confirm "system clock" and "sandbox mode" as
   testability tactics.)

## How architecture helps

- Hexagonal architecture is explicitly motivated by testability: Cockburn's intent is to let
  the application be driven by users, programs, automated tests or batch scripts and be
  developed and tested in isolation from run-time devices and databases, using adapters
  such as a mock database (see `style-hexagonal`).
- Seams at boundaries (ports, interfaces, injected clocks and random sources) turn hidden
  dependencies into controllable inputs.

## Use when

- Tests need a database, network or wall-clock and are slow or flaky.
- Defining a testability scenario for a critical component.
- Choosing where to place interfaces so a fake can replace an external dependency.

## Do not use when

- Adding test-only hooks that change production behavior or widen the attack surface without
  review; prefer boundary seams over debug back doors.
- Splitting code into interfaces solely to mock everything. Mocks of your own internals bind
  tests to structure; mock at real boundaries.
- Chasing a coverage number as the target. Coverage is a measure, not the quality.

## Trade-offs

- Extra interfaces and adapters add indirection (see `quality-modifiability`).
- Record/playback and executable assertions add runtime overhead unless disabled in
  production.
- Determinism (fixed clock, seeded randomness) may hide timing bugs; keep some
  non-deterministic integration tests.

## Common mistakes

- Static or global access to time, environment and singletons.
- Treating testability as a test-team activity; it is a design property fixed early.

## Verification note

Confidence is medium: tactic names come from the SAiP table of contents, the book's
acknowledgments, and secondary summaries, not from the chapter text. Re-verify against the
4th edition chapter when a copy is accessible through legitimate means.

## Related

- Notes: `style-hexagonal`, `quality-modifiability`, `quality-overview-25010`
- Skills: `tdd-red-green-refactor`, `tdd-arrange-act-assert`, `solid-dependency-inversion`

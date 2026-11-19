---
name: principle-coupling-cohesion
description: Coupling and cohesion - the classic pair for judging module boundaries, how the SEI ties them to the cost of change, and how afferent/efferent counts make coupling measurable
domain: software-architecture
tags: principle,coupling,cohesion,modularity,metrics
apply_when: "judging whether a boundary is good, explaining why a change is expensive, or deciding whether to split or merge modules"
sources: "Bachmann, Bass, Nord - Modifiability Tactics, CMU/SEI-2007-TR-002, sections 3.1 and 5 (quotes Stevens, Myers, Constantine, Structured Design, 1974); Martin - Design Principles and Design Patterns (2000), package coupling metrics Ca, Ce, I; ISO/IEC 25010:2023 modularity (via secondary sources); Fowler - Refactoring, 2nd ed. (2018), ch. 3 (Divergent Change, Shotgun Surgery)"
last_reviewed: 2026-09-21
confidence: medium
---

# Coupling and cohesion

## Definitions

- **Coupling** (Stevens, Myers, Constantine 1974, quoted in the SEI report): the measure of
  the strength of association established by a connection from one module to another;
  coupling is reduced when relationships among elements not in the same module are
  minimized.
- **Cohesion**: a measure of the relationship among the responsibilities of a specific
  module (SEI wording). High cohesion: the responsibilities belong together; a change to
  one is likely to involve the others.
- ISO 25010:2023 names the goal "modularity": a system composed of discrete components so
  that a change to one has minimal impact on others (secondary source wording).

## Why they matter (SEI)

The SEI report justifies its modifiability tactics with these two concepts and with cost:
split responsibilities that do not belong together; increase cohesion (keep semantic
coherence, factor common services); reduce coupling (encapsulation, wrapper, higher
abstraction level, intermediary with restricted communication paths). Cost of a change is
driven by how many modules must change and how far the effect spreads.
See `quality-modifiability`.

## Measuring coupling at package level (Martin 2000)

- Ca (afferent): classes outside the package that depend on classes inside it (incoming).
- Ce (efferent): classes inside the package that depend on classes outside it (outgoing).
- Instability I = Ce / (Ca + Ce), range 0 to 1: 0 means no outgoing dependencies (stable),
  1 means no incoming dependencies (instable).
- Martin notes these metrics are imperfect and relying on them alone would be foolhardy.
  Details in `principle-component-cohesion-coupling`.

## Symptoms in code (Fowler smells)

| Symptom | Reads as |
|---|---|
| Divergent Change: one module changes for unrelated reasons | low cohesion |
| Shotgun Surgery: one change touches many modules | logic scattered, high coupling between modules |
| Feature Envy, Message Chains, Insider Trading | excess coupling across a boundary |

## Use when

- Reviewing a proposed boundary: ask what changes together (cohesion) and what must know
  about what (coupling).
- Explaining a costly change in terms of which dependency spread it.

## Do not use when

- Treating them as absolutes. Zero coupling is not the goal; a system's parts must talk.
  The goal is coupling that is intentional, minimal and through stable interfaces.
- Reading a single metric as a verdict. Package metrics ignore what the dependency is; a
  dependency on a stable abstraction differs from a dependency on a volatile class.
- Splitting purely to raise "cohesion" scores: more, smaller modules increase interfaces
  and coordination.

## Trade-offs

- Lower coupling usually means more interfaces, indirection and mapping code.
- Higher cohesion may fragment a feature across modules, so one user-visible change touches
  several cohesive units; choose the grouping by the changes you expect (see the common
  closure principle in `principle-component-cohesion-coupling`).

## Common mistakes

- Counting only compile-time dependencies; shared database tables, shared files and
  runtime message formats are couplings too.
- Grouping by technical type (all controllers, all repositories) and then paying for every
  feature change across groups.

## Verification note

Definitions of coupling and cohesion were read as quoted in the SEI report, not from the
1974 paper. ISO and Fowler wording came from secondary sources. The classical ordered scales
of coupling and cohesion types from the 1974 paper were not read and are not stated here.

## Related

- Notes: `principle-information-hiding`, `principle-component-cohesion-coupling`,
  `quality-modifiability`, `principle-layering-dependency-rule`
- Notes (software-design): `cleancode-code-smells`
- Skills: `solid-single-responsibility`, `solid-dependency-inversion`

---
name: style-microkernel
description: Microkernel (plug-in) architecture - a minimal core plus independent plug-ins behind a registry and contracts; fits products and volatile variation points, poor fit for high scale
domain: software-architecture
tags: architecture,style,microkernel,plugin,extensibility
apply_when: "a product must grow by adding features, or one volatile area (rules, integrations, formats) should be isolated so it can change without touching a stable core"
sources: "Richards - Software Architecture Patterns, O'Reilly report, 2015, Microkernel Architecture (read); Fowler - Patterns of Enterprise Application Architecture, Plugin pattern, martinfowler.com catalog entry (read, related but different scope)"
last_reviewed: 2026-09-21
confidence: medium
---

# Microkernel (plug-in) architecture

## Intent

Keep a small, stable core system and put variable features and custom processing into
independent plug-in modules, so features can be added, removed or changed with little effect
on the core or on each other.

## Structure (Richards)

- **Core system**: the minimal functionality to make the system operational; in a business
  application, the general business logic without special cases or custom rules.
- **Plug-in modules**: stand-alone components with specialized processing or features.
  Generally independent of each other; keep communication between plug-ins minimal to avoid
  dependency problems.
- **Plug-in registry**: the core knows which plug-ins exist and how to reach them: name,
  data contract, access protocol details.
- **Connection**: not fixed by the pattern (Richards lists OSGi, messaging, web services, or
  direct binding by object instantiation); only independence between plug-ins is required.
- **Contracts**: standard or custom. For third-party plug-ins you do not control, put an
  adapter between the plug-in's contract and your standard one so the core has no special
  code per plug-in. Decide a contract versioning strategy at the start.
- Examples given: Eclipse IDE, web browsers (product-based), and an insurance claims system
  where each state's rules is a plug-in around a core claims process.
- Can be embedded in another style to isolate one volatile area, for instance inside a
  layered application.

## Scorecard (Richards, typical implementation)

Agility High, ease of deployment High (plug-ins can be added at runtime in some
implementations), testability High (plug-ins tested in isolation), performance High (you can
strip unused features), scalability Low (usually single-unit products), ease of development
Low (contract governance, registry, granularity, connectivity choices). One author's
judgement of the typical case.

## Use when

- Product-style software with versions, optional features and control over who gets which
  feature.
- A rules or variant explosion (per country, per customer, per format) where each variant
  can be separated from the general flow.
- Third parties or other teams should extend the application without changing its core.

## Do not use when

- The main goal is scalable throughput of a large, uniform workload; the style is not known
  for scalability (Richards).
- Variation is small and stable: a strategy interface or configuration is simpler than a
  registry, contracts and versioning.
- The core would need to know plug-in internals, or plug-ins constantly call each other: the
  independence that gives the benefits is lost.
- The extension points are unknown; design them from real variation, not speculation (see
  `quality-modifiability` on when not to prepare for change).

## Trade-offs

| Gain | Cost |
|---|---|
| Change isolated to a plug-in; stable core | Registry, contract design and versioning are real work |
| Runtime add/remove of features in some designs | Plug-in granularity is a hard design decision |
| Easy prototyping by mocking plug-ins | Limited scaling; complexity when plug-ins interact |

## Common mistakes

- Plug-ins that reach into each other or into core internals.
- A core that keeps growing until it contains the special cases it was meant to exclude.
- Skipping version rules for the plug-in contract, then breaking every plug-in on a core
  upgrade.

## Verification note

The structure and scorecard come from one source (Richards, 2015). Fowler's Plugin pattern
concerns runtime-configured linking of implementations (for example in-memory versus database
components) and is a related mechanism, not the same architecture. No independent source was
read for the architecture-level claims, hence `confidence: medium`.

## Related

- Notes: `style-layered`, `style-modular-monolith`, `style-choosing`,
  `quality-modifiability`
- Skills: `solid-open-closed`
- Notes (software-design): `pattern-strategy`
